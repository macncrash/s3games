#include "game/orchard.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace orchard {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float FOCAL = 230.0f;
constexpr float HORIZON = 86.0f;
constexpr float GROUND = 2.05f;
constexpr float LANE = 1.15f;
constexpr float Z_SPAWN = 34.0f;
constexpr float Z_GATE = 4.4f;
constexpr float Z_HIT = 9.2f;
constexpr float MOVE = 2.6f;
constexpr float BELL_AT = 36.0f;
constexpr float BELL_END = 50.0f;
constexpr float ROPE_NEED = 1.05f;

struct Arr {
    float t;
    int kind;
    int lane;
};
const Arr kArr[] = {
    {6.0f, 0, -1}, {9.2f, 0, 1},  {12.4f, 0, 0}, {15.6f, 1, -1}, {19.2f, 0, 1},
    {22.6f, 0, 0}, {26.0f, 0, -1}, {29.4f, 1, 1}, {40.0f, 1, -1},
};

float speedOf(int kind) { return kind == 0 ? 5.4f : 3.5f; }
int ptsOf(int kind) { return kind == 0 ? 100 : 220; }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Watch && bell_) return 2;
    if (mode_ == Mode::Watch) return 1;
    return 0;
}

void Game::project(float worldX, float z, float& sx, float& sy, float& s) const {
    float zz = std::max(0.85f, z);
    s = FOCAL / zz;
    sx = 160.0f + worldX * s;
    sy = HORIZON + GROUND * s;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 24 || s.y + s.h < -48) return;
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
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y + g.h * scale * 0.5f, g.h * scale, pal, false, 0, false);
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

void Game::winWatch() {
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE WATCH HELD UNTIL THE RELIEF BELL";
    score_ += 1000 + baskets_ * 200;
    endT_ = 0;
    sys_->apu.tone(0, 660, 0.2f);
}

void Game::loseWatch(const char* why) {
    mode_ = Mode::Over;
    won_ = false;
    reason_ = why;
    endT_ = 0;
    sys_->apu.tone(0, 110, 0.22f);
}

void Game::beginWatch() {
    mode_ = Mode::Watch;
    over_ = false;
    won_ = false;
    bell_ = false;
    reason_ = "WATCH OVER";
    score_ = 0;
    baskets_ = 3;
    spawnAt_ = 0;
    t_ = 0;
    watch_ = 0;
    px_ = 0;
    swing_ = 0;
    rope_ = 0;
    shake_ = 0;
    endT_ = 0;
    foes_.clear();
}

void Game::botThink() {
    wantSwing_ = false;
    wantRope_ = false;
    move_ = 0;
    const Foe* urgent = nullptr;
    for (const Foe& f : foes_) {
        if (!f.alive || f.z > 16.0f) continue;
        if (!urgent || f.z < urgent->z) urgent = &f;
    }
    if (urgent) {
        float target = float(urgent->lane);
        if (px_ < target - 0.08f) move_ = 1;
        else if (px_ > target + 0.08f) move_ = -1;
        if (std::fabs(px_ - target) < 0.35f && urgent->z < Z_HIT) wantSwing_ = true;
        return;
    }
    if (px_ < -0.08f) move_ = 1;
    else if (px_ > 0.08f) move_ = -1;
    if (bell_ && std::fabs(px_) < 0.28f) wantRope_ = true;
}

void Game::update(float dt) {
    watch_ += dt;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - dt);
    if (swing_ > 0) swing_ = std::max(0.0f, swing_ - dt);

    if (!bell_ && watch_ >= BELL_AT) {
        bell_ = true;
        sys_->apu.tone(1, 520, 0.16f);
    }
    if (bell_ && watch_ < BELL_END) {
        float ph = std::fmod(watch_ * 2.2f, 1.0f);
        if (ph < dt * 2.4f) sys_->apu.tone(1, 494, 0.1f);
    }

    while (spawnAt_ < int(sizeof(kArr) / sizeof(kArr[0])) && watch_ >= kArr[spawnAt_].t) {
        Foe f;
        f.kind = kArr[spawnAt_].kind;
        f.lane = kArr[spawnAt_].lane;
        f.z = Z_SPAWN;
        f.speed = speedOf(f.kind);
        foes_.push_back(f);
        spawnAt_++;
    }

    px_ = std::clamp(px_ + move_ * MOVE * dt, -1.15f, 1.15f);

    if (wantSwing_ && swing_ <= 0) {
        swing_ = 0.22f;
        sys_->apu.tone(0, 180, 0.12f);
        for (Foe& f : foes_) {
            if (!f.alive) continue;
            if (std::fabs(px_ - float(f.lane)) > 0.42f) continue;
            if (f.z > Z_HIT || f.z < Z_GATE) continue;
            f.alive = false;
            score_ += ptsOf(f.kind);
            shake_ = 0.08f;
        }
    }

    for (Foe& f : foes_) {
        if (!f.alive) continue;
        f.z -= f.speed * dt;
        if (f.z <= Z_GATE) {
            f.alive = false;
            baskets_ -= 1;
            shake_ = 0.2f;
            sys_->apu.noiseBurst(0.25f, 1800, 0.15f);
            if (baskets_ <= 0) {
                loseWatch("THE BASKETS ARE GONE");
                return;
            }
        }
    }

    if (bell_ && wantRope_ && std::fabs(px_) < 0.32f) rope_ += dt;
    else rope_ = std::max(0.0f, rope_ - dt * 1.6f);

    if (rope_ >= ROPE_NEED) {
        winWatch();
        return;
    }
    if (bell_ && watch_ >= BELL_END) loseWatch("MISSED THE BELL");
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float dawn = 0.15f;
    if (mode_ == Mode::Watch) dawn = 0.15f + std::clamp(watch_ / BELL_AT, 0.0f, 1.0f) * 0.45f;
    if (bell_ || mode_ == Mode::Victory) dawn = std::max(dawn, 0.72f);
    uint16_t skyTop = lerpC(gs::rgb4(1, 1, 4), gs::rgb4(6, 8, 12), dawn);
    uint16_t skyHor = lerpC(gs::rgb4(6, 3, 3), gs::rgb4(14, 9, 5), dawn);
    v.setFogColor(lerpC(gs::rgb4(2, 3, 4), gs::rgb4(10, 8, 6), dawn * 0.6f));

    float shx = shake_ > 0 ? std::sin(t_ * 80.0f) * 4.0f * shake_ : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            v.lineBackdrop[y] = lerpC(skyTop, skyHor, y / HORIZON);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - int(HORIZON)) + 1.0f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160.0f + shx;
        r.hw = 28.0f + row * 1.15f;
        r.v = 2400.0f / row + watch_ * 18.0f;
        r.pal = PAL_FLOOR;
        r.band = (int(r.v / 40.0f) & 1) ? 1 : 0;
        r.style = 0;
        r.left = r.right = 0;
        v.lineBackdrop[y] = gs::rgb4(1, 3, 1);
        v.lineFog[y] = uint8_t(std::clamp(int(10.0f - row * 0.12f), 0, 8));
    }

    auto fogOf = [](float z) { return int(std::clamp((z - 10.0f) * 0.45f, 0.0f, 12.0f)); };

    for (int i = 8; i >= 0; --i) {
        float z = 7.0f + float(i) * 3.4f;
        int fog = fogOf(z);
        for (int side = -1; side <= 1; side += 2) {
            for (int lane = -1; lane <= 1; lane++) {
                float x = float(lane) * LANE + float(side) * 0.55f;
                float sx, sy, s;
                project(x, z, sx, sy, s);
                spr(art_.shadow, sx, sy, 8.0f * s * 0.15f, PAL_TREE, false, fog, false);
                spr(art_.tree, sx, sy, 70.0f * s * 0.22f, PAL_TREE, side < 0, fog, true);
            }
        }
    }

    for (int lane = -1; lane <= 1; lane++) {
        if (lane >= baskets_ - 1 && baskets_ < 3 && lane != 0) {
            // dim missing edge baskets by skipping when spoiled from the outside
        }
        int shown = 3;
        if (baskets_ <= 0) shown = 0;
        else if (baskets_ == 1 && lane != 0) continue;
        else if (baskets_ == 2 && lane == 1) continue;
        (void)shown;
        float sx, sy, s;
        project(float(lane) * LANE * 0.72f, 5.1f, sx, sy, s);
        spr(art_.basket, sx, sy - 4, 22.0f * s * 0.28f, PAL_BASK, false, 0, true);
    }

    float bsx, bsy, bs;
    project(0, 8.5f, bsx, bsy, bs);
    float swing = bell_ ? std::sin(watch_ * 9.0f) * 10.0f : std::sin(t_ * 1.4f) * 2.0f;
    spr(art_.rope, bsx, bsy - 36.0f * bs * 0.2f, 40.0f * bs * 0.18f, PAL_BELL, false, 1, false);
    spr(art_.bell, bsx + swing, bsy - 18.0f * bs * 0.2f, 32.0f * bs * 0.22f, PAL_BELL, false, 1, false);

    std::vector<int> order;
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[i].alive) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[a].z > foes_[b].z; });
    for (int i : order) {
        const Foe& f = foes_[i];
        float sx, sy, s;
        project(float(f.lane) * LANE, f.z, sx, sy, s);
        int fog = fogOf(f.z);
        spr(art_.shadow, sx, sy, 10.0f * s * 0.16f, PAL_CROW, false, fog, false);
        if (f.kind == 0) {
            int fr = int(t_ * 8.0f + f.z) & 1;
            spr(art_.crow[fr], sx, sy - 18.0f * s * 0.25f, 24.0f * s * 0.32f, PAL_CROW, f.lane < 0, fog, false);
        } else {
            spr(art_.picker, sx, sy, 56.0f * s * 0.24f, PAL_PICK, f.lane > 0, fog, true);
        }
    }

    if (mode_ != Mode::Title) {
        float sx, sy, s;
        project(px_ * LANE, 3.15f, sx, sy, s);
        spr(art_.shadow, sx, sy, 14.0f * s * 0.14f, PAL_KEEP, false, 0, false);
        spr(art_.keep[swing_ > 0 ? 1 : 0], sx, sy, 64.0f * s * 0.26f, PAL_KEEP, false, 0, true);
    } else {
        float sx, sy, s;
        project(0, 3.3f, sx, sy, s);
        spr(art_.keep[int(t_ * 2.0f) & 1], sx, sy, 64.0f * s * 0.26f, PAL_KEEP, false, 0, true);
    }

    hud(1, 1, "ORCHARD", 1);
    hud(28, 1, "SCORE " + std::to_string(score_), 1);
    if (mode_ == Mode::Watch) {
        hud(1, 2, "BASKETS " + std::to_string(baskets_), baskets_ > 1 ? 5 : 4);
        if (!bell_) {
            int left = int(std::ceil(BELL_AT - watch_));
            hudC(25, "HOLD  BELL " + std::to_string(left), 3);
        } else {
            int pct = int(std::clamp(rope_ / ROPE_NEED, 0.0f, 1.0f) * 8.0f);
            std::string bar(size_t(pct), '#');
            hudC(25, "PULL THE ROPE " + bar, 3);
        }
        hudC(26, "ARROWS MOVE   A SWING   B ROPE", 2);
    } else if (mode_ == Mode::Title) {
        text("ORCHARD RELIEF", 160, 28, 1.15f, PAL_HUD, 0);
        text("HOLD UNTIL THE BELL", 160, 52, 0.7f, PAL_HUD, 0);
        if (int(t_ * 2) & 1) text("PRESS START", 160, 168, 0.8f, PAL_HUD, 0);
        hudC(26, "MISS THE BELL AND THE WATCH IS OVER", 2);
    } else if (mode_ == Mode::Victory) {
        text("RELIEF ANSWERED", 160, 36, 1.0f, PAL_HUD, 0);
        text(reason_, 160, 64, 0.42f, PAL_HUD, 0);
        hudC(26, "THE WATCH HELD", 5);
    } else if (mode_ == Mode::Over) {
        text("WATCH OVER", 160, 36, 1.1f, PAL_HUD, 0);
        text(reason_, 160, 64, 0.55f, PAL_HUD, 0);
        hudC(26, "PRESS START", 4);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(4, 5, 6));
    sys.apu.setEcho(0.22f, 0.28f, 0.18f);
    sys.apu.setMaster(0.8f);
    mode_ = Mode::Title;
    if (bot_) beginWatch();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    gs::Pad& pad = sys.pad;
    wantSwing_ = false;
    wantRope_ = false;
    move_ = 0;

    if (mode_ == Mode::Title) {
        if (bot_) beginWatch();
        else if (pad.pressed(gs::BTN_START)) beginWatch();
    } else if (mode_ == Mode::Watch) {
        if (bot_) botThink();
        else {
            if (pad.down(gs::BTN_LEFT)) move_ = -1;
            if (pad.down(gs::BTN_RIGHT)) move_ = 1;
            float ax = pad.axisX;
            if (std::fabs(ax) > 0.25f) move_ = ax > 0 ? 1 : -1;
            wantSwing_ = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C);
            wantRope_ = pad.down(gs::BTN_B);
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Mode::Title;
            }
        }
        if (mode_ == Mode::Watch) update(DT);
    } else {
        endT_ += DT;
        if (endT_ > 0.7f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
        }
    }
    draw();
}

}  // namespace orchard

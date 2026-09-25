#include "game/militia.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

#include "version.h"

namespace militia {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kWellX = 160.f;
constexpr float kWellY = 118.f;
constexpr float kStand = 64.f;
constexpr float kReach = 30.f;
constexpr float kBody = 26.f;
constexpr float kSpeed = 165.f;
constexpr float kShot = 320.f;
constexpr float kReload = 0.30f;
constexpr int kWellMax = 10;
constexpr float kPI = 3.14159265f;

struct KindDef {
    int hp, dmg, pts, pal;
    float speed, rad, tall;
};

const KindDef kKind[3] = {
    {1, 1, 100, PAL_RUN, 30.f, 10.f, 30.f},
    {2, 1, 150, PAL_RAID, 22.f, 11.f, 34.f},
    {2, 2, 250, PAL_TORCH, 16.f, 11.f, 36.f},
};

float wrap(float a) {
    while (a > kPI) a -= kPI * 2.f;
    while (a < -kPI) a += kPI * 2.f;
    return a;
}

void gatePos(int gate, float j, float& x, float& y) {
    float s = 14.f * j;
    if (gate == 0) {
        x = kWellX + s;
        y = 18.f;
    } else if (gate == 1) {
        x = 296.f;
        y = kWellY + s;
    } else if (gate == 2) {
        x = kWellX + s;
        y = 208.f;
    } else {
        x = 24.f;
        y = kWellY + s;
    }
}

void facing(float fx, float fy, int& dir, bool& flip) {
    flip = false;
    if (std::fabs(fx) >= std::fabs(fy)) {
        dir = 2;
        flip = fx < 0;
    } else {
        dir = fy >= 0.f ? 0 : 1;
    }
}

const gs::Mipped& foeSprite(const Art& a, int kind, int dir, int step) {
    if (kind <= 0) return a.run[dir][step];
    if (kind == 1) return a.raid[dir][step];
    return a.torch[dir][step];
}

}  // namespace

int Game::marker() const {
    if (over_) return 4;
    if (mode_ == Mode::Clear) return 2;
    if (mode_ == Mode::Fight && wave_ == 2) return 3;
    if (mode_ == Mode::Fight || mode_ == Mode::Pause) return 1;
    return 0;
}

const char* Game::waveName() const {
    static const char* n[3] = {"THE LANE", "THE YARD", "THE TORCH"};
    int w = wave_;
    if (w < 0) w = 0;
    if (w > 2) w = 2;
    return n[w];
}

const char* Game::waveOrder() const {
    static const char* n[3] = {"EIGHT ON THE NORTH ROAD", "EAST GATE, THEN THE WEST", "SOUTH TORCHES, THEN THE RUSH"};
    int w = wave_;
    if (w < 0) w = 0;
    if (w > 2) w = 2;
    return n[w];
}

void Game::blip(bool high) {
    sys_->apu.tone(2, high ? 880.f : 520.f, 0.06f);
    blip_ = 0.05f;
}

void Game::fanfare(bool big) {
    fanStep_ = 0;
    fanT_ = 0;
    fanBig_ = big;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::shadowAt(float x, float y, float w) {
    if (w < 4.f) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::max(3, int(std::lround(w * 0.42f))));
    s.x = int16_t(std::lround(x - s.w * 0.5f));
    s.y = int16_t(std::lround(y - s.h * 0.5f));
    s.img = art_.shadow.pick(float(s.h));
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::applySky() {
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        sys_->vdp.lineBackdrop[y] = sky_;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.setFogColor(sky_);
}

void Game::takePost() {
    if (spawns_.empty()) {
        px_ = kWellX;
        py_ = kWellY + kStand;
        faceX_ = 0;
        faceY_ = -1;
        return;
    }
    float x, y;
    gatePos(spawns_[0].gate, spawns_[0].j, x, y);
    float a = std::atan2(y - kWellY, x - kWellX);
    px_ = kWellX + std::cos(a) * kStand;
    py_ = kWellY + std::sin(a) * kStand;
    faceX_ = std::cos(a);
    faceY_ = std::sin(a);
}

void Game::prepareWave() {
    foes_.clear();
    shots_.clear();
    puffs_.clear();
    pops_.clear();
    spawns_.clear();
    cursor_ = 0;
    reload_ = 0;
    auto add = [&](float t, int kind, int gate, float j) { spawns_.push_back({t, kind, gate, j}); };
    if (wave_ <= 0) {
        for (int i = 0; i < 8; ++i) add(0.55f + i * 0.82f, 0, 0, (i % 2 ? 0.7f : -0.7f));
    } else if (wave_ == 1) {
        for (int i = 0; i < 5; ++i) add(0.45f + i * 1.05f, 1, 1, (i % 2 ? 0.65f : -0.55f));
        for (int i = 0; i < 4; ++i) add(7.2f + i * 1.05f, (i % 2) ? 0 : 1, 3, (i % 2 ? -0.6f : 0.5f));
    } else {
        for (int i = 0; i < 4; ++i) add(0.45f + i * 1.25f, 2, 2, (i - 1.5f) * 0.45f);
        for (int i = 0; i < 6; ++i) add(8.0f + i * 0.8f, 0, 0, (i % 3 - 1) * 0.55f);
    }
    yardTint(sys_->vdp, wave_, sky_);
    applySky();
    takePost();
}

void Game::boot() {
    wave_ = 0;
    score_ = 0;
    well_ = kWellMax;
    won_ = false;
    over_ = false;
    muzzle_ = 0;
    shake_ = 0;
    hurtFlash_ = 0;
    prepareWave();
    mode_ = Mode::Brief;
    t_ = 0;
    fanStep_ = -1;
}

void Game::fireShot() {
    if (reload_ > 0 || mode_ != Mode::Fight) return;
    float fl = std::sqrt(faceX_ * faceX_ + faceY_ * faceY_);
    if (fl < 0.2f) return;
    faceX_ /= fl;
    faceY_ /= fl;
    reload_ = kReload;
    muzzle_ = 0.07f;
    Shot s;
    s.x = px_ + faceX_ * 14.f;
    s.y = py_ + faceY_ * 14.f;
    s.vx = faceX_ * kShot;
    s.vy = faceY_ * kShot;
    s.life = 0.85f;
    shots_.push_back(s);
    sys_->apu.noiseBurst(0.32f, 2200.f, 0.07f);
    sys_->apu.tone(1, 150.f, 0.07f);
    shotTone_ = 0.05f;
}

void Game::damageFoe(int i, int dmg) {
    if (i < 0 || i >= int(foes_.size())) return;
    Foe& f = foes_[i];
    f.hp -= dmg;
    f.flash = 0.12f;
    puffs_.push_back({f.x, f.y, 0.22f, f.tall * 0.7f});
    if (f.hp > 0) {
        sys_->apu.noiseBurst(0.18f, 1600.f, 0.05f);
        return;
    }
    score_ += f.pts;
    pops_.push_back({f.x, f.y - 10.f, 0.7f, f.pts});
    if (pops_.size() > 4) pops_.erase(pops_.begin());
    sys_->apu.noiseBurst(0.28f, 900.f, 0.08f);
    foes_.erase(foes_.begin() + i);
}

void Game::swingButt() {
    if (butt_ > 0 || mode_ != Mode::Fight) return;
    butt_ = 0.48f;
    sys_->apu.tone(1, 90.f, 0.08f);
    shotTone_ = 0.06f;
    int best = -1;
    float bestD = 22.f * 22.f;
    for (int i = 0; i < int(foes_.size()); ++i) {
        float dx = foes_[i].x - px_, dy = foes_[i].y - py_;
        float d = dx * dx + dy * dy;
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    if (best >= 0) damageFoe(best, 1);
}

void Game::breach(int dmg) {
    if (mode_ != Mode::Fight) return;
    well_ -= dmg;
    if (well_ < 0) well_ = 0;
    shake_ = 1.f;
    hurtFlash_ = 0.5f;
    sys_->rumble(0.7f, 1.f, 180);
    sys_->apu.noiseBurst(0.5f, 420.f, 0.2f);
    if (well_ > 0) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    hurtFlash_ = 2.5f;
}

void Game::win() {
    if (won_) return;
    score_ += well_ * 100;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    fanfare(true);
}

void Game::steer(float tx, float ty, float dt) {
    float dx = tx - px_, dy = ty - py_;
    float dist = std::sqrt(dx * dx + dy * dy);
    if (dist < 0.4f) {
        moved_ = false;
        return;
    }
    float step = std::min(kSpeed * dt, dist);
    float vx = dx / dist, vy = dy / dist;
    float nx = px_ + vx * step;
    float ny = py_ + vy * step;
    float wx = nx - kWellX, wy = ny - kWellY;
    float wd = std::sqrt(wx * wx + wy * wy);
    float ox = px_, oy = py_;
    if (wd >= kBody) {
        px_ = nx;
        py_ = ny;
    } else {
        float a0 = std::atan2(py_ - kWellY, px_ - kWellX);
        float a1 = std::atan2(ty - kWellY, tx - kWellX);
        float da = wrap(a1 - a0);
        float r = std::sqrt((px_ - kWellX) * (px_ - kWellX) + (py_ - kWellY) * (py_ - kWellY));
        if (r < kBody) r = kBody;
        float angStep = step / r;
        float a = a0 + std::clamp(da, -angStep, angStep);
        float tr = std::sqrt((tx - kWellX) * (tx - kWellX) + (ty - kWellY) * (ty - kWellY));
        if (tr < kBody + 1.f) tr = kBody + 1.f;
        r += std::clamp(tr - r, -step, step);
        if (r < kBody) r = kBody;
        px_ = kWellX + std::cos(a) * r;
        py_ = kWellY + std::sin(a) * r;
    }
    px_ = std::clamp(px_, 12.f, 308.f);
    py_ = std::clamp(py_, 14.f, 210.f);
    float mx = px_ - ox, my = py_ - oy;
    float md = std::sqrt(mx * mx + my * my);
    moved_ = md > 0.15f;
    if (moved_) {
        faceX_ = mx / md;
        faceY_ = my / md;
    }
}

void Game::botAct(float dt) {
    float tx = kWellX, ty = kWellY - kStand;
    int fi = -1;
    float best = 1e12f;
    for (int i = 0; i < int(foes_.size()); ++i) {
        float dx = foes_[i].x - kWellX, dy = foes_[i].y - kWellY;
        float d = dx * dx + dy * dy;
        if (d < best) {
            best = d;
            fi = i;
        }
    }
    bool live = fi >= 0;
    if (live) {
        tx = foes_[fi].x;
        ty = foes_[fi].y;
    } else if (cursor_ < int(spawns_.size())) {
        gatePos(spawns_[cursor_].gate, spawns_[cursor_].j, tx, ty);
    }
    float ang = std::atan2(ty - kWellY, tx - kWellX);
    float cur = std::atan2(py_ - kWellY, px_ - kWellX);
    float da = wrap(ang - cur);
    float step = (kSpeed / kStand) * dt;
    float a = cur + std::clamp(da, -step, step);
    float ox = px_, oy = py_;
    px_ = kWellX + std::cos(a) * kStand;
    py_ = kWellY + std::sin(a) * kStand;
    float mx = px_ - ox, my = py_ - oy;
    float md = std::sqrt(mx * mx + my * my);
    moved_ = md > 0.2f;
    if (moved_) {
        faceX_ = mx / md;
        faceY_ = my / md;
    } else {
        faceX_ = std::cos(ang);
        faceY_ = std::sin(ang);
    }
    if (!live) return;
    if (std::fabs(da) < 0.55f) {
        float dx = tx - px_, dy = ty - py_;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d > 1.f) {
            faceX_ = dx / d;
            faceY_ = dy / d;
        }
        if (std::fabs(da) < 0.2f) fireShot();
        if (d < 20.f) swingButt();
    }
}

void Game::humanAct(float dt) {
    const gs::Pad& pad = sys_->pad;
    float ix = (pad.down(gs::BTN_RIGHT) ? 1.f : 0.f) - (pad.down(gs::BTN_LEFT) ? 1.f : 0.f);
    float iy = (pad.down(gs::BTN_DOWN) ? 1.f : 0.f) - (pad.down(gs::BTN_UP) ? 1.f : 0.f);
    if (std::fabs(pad.axisX) > 0.2f && ix == 0.f) ix = pad.axisX;
    moved_ = false;
    if (ix != 0.f || iy != 0.f) {
        float d = std::sqrt(ix * ix + iy * iy);
        steer(px_ + ix / d * 48.f, py_ + iy / d * 48.f, dt);
    }
    if (pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO) || pad.accel > 0.4f) fireShot();
    if (pad.down(gs::BTN_B) || pad.down(gs::BTN_X) || pad.brake > 0.4f) swingButt();
}

void Game::update(float dt) {
    if (mode_ != Mode::Fight) return;
    if (reload_ > 0) reload_ -= dt;
    if (butt_ > 0) butt_ -= dt;
    if (bot_) botAct(dt);
    else humanAct(dt);

    while (cursor_ < int(spawns_.size()) && t_ >= spawns_[cursor_].t) {
        const Spawn& s = spawns_[cursor_++];
        int k = s.kind;
        if (k < 0) k = 0;
        if (k > 2) k = 2;
        Foe f;
        f.kind = k;
        gatePos(s.gate, s.j, f.x, f.y);
        f.speed = kKind[k].speed;
        f.rad = kKind[k].rad;
        f.tall = kKind[k].tall;
        f.age = 0;
        f.flash = 0;
        f.hp = kKind[k].hp;
        f.dmg = kKind[k].dmg;
        f.pts = kKind[k].pts;
        foes_.push_back(f);
    }

    for (int i = 0; i < int(foes_.size());) {
        Foe& f = foes_[i];
        float dx = kWellX - f.x, dy = kWellY - f.y;
        float d = std::sqrt(dx * dx + dy * dy);
        f.age += dt;
        if (f.flash > 0) f.flash -= dt;
        if (d <= kReach) {
            int dmg = f.dmg;
            foes_.erase(foes_.begin() + i);
            breach(dmg);
            if (mode_ != Mode::Fight) return;
            continue;
        }
        f.x += dx / d * f.speed * dt;
        f.y += dy / d * f.speed * dt;
        d = std::sqrt((kWellX - f.x) * (kWellX - f.x) + (kWellY - f.y) * (kWellY - f.y));
        if (d <= kReach) {
            int dmg = f.dmg;
            foes_.erase(foes_.begin() + i);
            breach(dmg);
            if (mode_ != Mode::Fight) return;
            continue;
        }
        ++i;
    }

    for (int i = 0; i < int(shots_.size());) {
        Shot& s = shots_[i];
        float x0 = s.x, y0 = s.y;
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.life -= dt;
        bool gone = s.life <= 0 || s.x < -30.f || s.x > 350.f || s.y < -30.f || s.y > 250.f;
        if (!gone) {
            for (int pass = 0; pass < 2 && !gone; ++pass) {
                float sx = pass ? s.x : (x0 + s.x) * 0.5f;
                float sy = pass ? s.y : (y0 + s.y) * 0.5f;
                for (int f = 0; f < int(foes_.size()); ++f) {
                    float dx = sx - foes_[f].x, dy = sy - foes_[f].y;
                    float r = foes_[f].rad + 4.f;
                    if (dx * dx + dy * dy <= r * r) {
                        damageFoe(f, 1);
                        gone = true;
                        break;
                    }
                }
            }
        }
        if (gone) shots_.erase(shots_.begin() + i);
        else ++i;
    }

    if (mode_ == Mode::Fight && cursor_ >= int(spawns_.size()) && foes_.empty()) {
        if (wave_ >= 2) win();
        else {
            mode_ = Mode::Clear;
            t_ = 0;
            fanfare(false);
        }
    }
}

void Game::fadeFx() {
    if (muzzle_ > 0) muzzle_ -= DT;
    if (shake_ > 0) shake_ -= DT;
    if (hurtFlash_ > 0 && mode_ != Mode::Over) hurtFlash_ -= DT;
    for (int i = 0; i < int(puffs_.size());) {
        puffs_[i].life -= DT;
        if (puffs_[i].life <= 0) puffs_.erase(puffs_.begin() + i);
        else ++i;
    }
    for (int i = 0; i < int(pops_.size());) {
        pops_[i].life -= DT;
        pops_[i].y -= 12.f * DT;
        if (pops_[i].life <= 0) pops_.erase(pops_.begin() + i);
        else ++i;
    }
}

void Game::serviceAudio() {
    if (shotTone_ > 0) {
        shotTone_ -= DT;
        if (shotTone_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (blip_ > 0) {
        blip_ -= DT;
        if (blip_ <= 0) sys_->apu.tone(2, 0, 0);
    }
    if (fanStep_ >= 0) {
        fanT_ += DT;
        if (fanT_ > 0.12f) {
            static const float notes[] = {392.f, 523.f, 659.f, 784.f};
            int n = fanBig_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0, 0);
            ++fanStep_;
            fanT_ = 0;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(0, 110.f, 0.03f);
    } else {
        sys_->apu.tone(0, 0, 0);
    }
    if (well_ <= 2) sys_->setLight(255, 40, 24);
    else if (well_ <= 5) sys_->setLight(220, 150, 40);
    else sys_->setLight(40, 170, 70);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(t_ * 80.f) * 3.5f * shake_;
        shy = std::cos(t_ * 63.f) * 2.5f * shake_;
    }

    auto body = [&](const gs::Mipped& m, float x, float y, float h, int pal, bool flip) {
        shadowAt(x + shx, y + h * 0.28f + shy, h * 0.85f);
        spr(m, x + shx, y + shy, h, pal, flip);
    };

    bool showCast = mode_ != Mode::Title;
    if (showCast || mode_ == Mode::Title) {
        for (int i = 0; i < 4; ++i) {
            float tx = (i % 2) ? 286.f : 34.f;
            float ty = (i / 2) ? 188.f : 52.f;
            spr(art_.tree, tx + shx, ty + shy, 56, PAL_TREE, i % 2, true);
        }
        spr(art_.bucket, 118 + shx, 156 + shy, 16, PAL_WELL, false);
        spr(art_.bucket, 206 + shx, 92 + shy, 14, PAL_WELL, true);
        body(art_.well, kWellX, kWellY, 82, PAL_WELL, false);
        float ga = t_ * 1.4f;
        spr(art_.glint, kWellX - 5.f + std::cos(ga) * 4.f + shx, kWellY - 2.f + std::sin(ga) * 2.f + shy, 8, PAL_FX, false);
        int cracks = 0;
        if (mode_ == Mode::Over) cracks = 3;
        else if (well_ < kWellMax) cracks = well_ <= 4 ? 2 : 1;
        for (int i = 0; i < cracks; ++i)
            spr(art_.crack, kWellX - 10.f + i * 12.f + shx, kWellY - 8.f + (i % 2) * 10.f + shy, 28 + i * 4.f, PAL_WELL, i == 1);
        if (mode_ == Mode::Over || hurtFlash_ > 0) {
            spr(art_.flash, kWellX - 8 + shx, kWellY - 4 + shy, 18 + std::sin(t_ * 14.f) * 3.f, PAL_FX, false);
            spr(art_.puff, kWellX + 6 + shx, kWellY - 12 + shy, 22, PAL_FX, false);
        }
    }

    if (mode_ == Mode::Title) {
        int step = int(t_ * 6.f) & 1;
        body(art_.raid[2][step], 250.f + std::sin(t_ * 1.2f) * 6.f, kWellY, 34, PAL_RAID, true);
        body(art_.mil[1][0], 160, 172, 38, PAL_MIL, false);
    }

    if (mode_ == Mode::Fight || mode_ == Mode::Pause || mode_ == Mode::Clear || mode_ == Mode::Brief ||
        mode_ == Mode::Victory || mode_ == Mode::Over) {
        int dir = 1;
        bool flip = false;
        facing(faceX_, faceY_, dir, flip);
        int step = moved_ ? (int(t_ * 9.f) & 1) : 0;
        body(art_.mil[dir][step], px_, py_, 38, PAL_MIL, flip);
        if (muzzle_ > 0)
            spr(art_.flash, px_ + faceX_ * 18.f + shx, py_ + faceY_ * 18.f + shy, 14, PAL_FX, false);
        for (const Shot& s : shots_) spr(art_.ball, s.x + shx, s.y + shy, 10, PAL_FX, false);
        for (int i = 0; i < int(foes_.size()); ++i) {
            const Foe& f = foes_[i];
            float dx = kWellX - f.x, dy = kWellY - f.y;
            int fd = 0;
            bool ff = false;
            facing(dx, dy, fd, ff);
            int st = int(f.age * 8.f) & 1;
            body(foeSprite(art_, f.kind, fd, st), f.x, f.y, f.tall, kKind[f.kind].pal, ff);
            if (f.flash > 0) spr(art_.flash, f.x + shx, f.y + shy, 12, PAL_FX, false);
        }
    }
    for (const Puff& p : puffs_) spr(art_.puff, p.x + shx, p.y + shy, p.h * (0.5f + p.life), PAL_FX, false);
    for (const Popup& p : pops_) text("+" + std::to_string(p.pts), p.x, p.y, 0.55f, PAL_GOLD);

    char buf[48];
    if (mode_ == Mode::Title) {
        text("S3 MILITIA", 160, 26, 1.15f, PAL_GOLD);
        text("THE WELL HAS TO STAND", 160, 50, 0.72f, PAL_WHITE);
        if ((int(t_ * 2.f) & 1) == 0) hudC(24, "PRESS START", PAL_GOLD);
        hudC(26, "ARROWS FACE   C FIRES   X BUTT", PAL_WHITE);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_WHITE);
    } else if (mode_ == Mode::Brief) {
        text(waveName(), 160, 22, 1.f, PAL_GOLD);
        hudC(4, waveOrder(), PAL_WHITE);
        std::snprintf(buf, sizeof buf, "WAVE %d OF 3", wave_ + 1);
        hudC(6, buf, PAL_WHITE);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "PRESS START", PAL_GOLD);
    } else if (mode_ == Mode::Fight || mode_ == Mode::Pause || mode_ == Mode::Clear) {
        std::snprintf(buf, sizeof buf, "WAVE %d OF 3", wave_ + 1);
        hud(1, 0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 0, buf, PAL_WHITE);
        hud(1, 1, "WELL", well_ <= 3 ? PAL_ALERT : PAL_WHITE);
        int pips = std::clamp(well_, 0, kWellMax);
        for (int i = 0; i < kWellMax; ++i) hud(6 + i, 1, i < pips ? "=" : "-", i < pips ? PAL_GOOD : PAL_ALERT);
        hud(30, 27, reload_ > 0.02f ? "LOAD" : "FIRE", reload_ > 0.02f ? PAL_ALERT : PAL_GOOD);
        if (mode_ == Mode::Pause) {
            text("PAUSED", 160, 78, 1.1f, PAL_WHITE);
            hudC(16, "START RESUMES", PAL_WHITE);
            hudC(18, "ESC TITLE", PAL_WHITE);
        } else if (mode_ == Mode::Clear) {
            text("WAVE CLEAR", 160, 36, 0.95f, PAL_GOOD);
        }
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 28, 0.85f, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(24, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "WELL %d", well_);
        hudC(26, buf, PAL_GOOD);
    } else if (mode_ == Mode::Over) {
        text("THE WELL FALLS", 160, 28, 0.9f, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(24, buf, PAL_WHITE);
        hudC(26, "START", PAL_WHITE);
    }

    fadeFx();
    serviceAudio();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    yardTint(sys.vdp, 0, sky_);
    applySky();
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.22f, 0.14f);
    px_ = 160;
    py_ = 172;
    faceX_ = 0;
    faceY_ = -1;
    well_ = kWellMax;
    if (bot_) boot();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (!bot_ && mode_ == Mode::Title) {
        px_ = 160;
        py_ = 172;
        faceX_ = 0;
        faceY_ = -1;
        moved_ = false;
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            boot();
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Brief) {
        if ((bot_ && t_ > 0.4f) || pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Fight;
            t_ = 0;
            blip(true);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            yardTint(sys.vdp, 0, sky_);
            applySky();
            well_ = kWellMax;
            foes_.clear();
            shots_.clear();
        }
    } else if (mode_ == Mode::Fight) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Fight;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            yardTint(sys.vdp, 0, sky_);
            applySky();
            well_ = kWellMax;
            foes_.clear();
            shots_.clear();
            spawns_.clear();
        }
    } else if (mode_ == Mode::Clear) {
        if ((bot_ && t_ > 0.45f) || pad.pressed(gs::BTN_START) || t_ > 2.2f) {
            ++wave_;
            prepareWave();
            mode_ = Mode::Brief;
            t_ = 0;
        }
    } else if (!bot_ && (mode_ == Mode::Victory || mode_ == Mode::Over)) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            well_ = kWellMax;
            foes_.clear();
            shots_.clear();
            puffs_.clear();
            pops_.clear();
            yardTint(sys.vdp, 0, sky_);
            applySky();
            t_ = 0;
            fanStep_ = -1;
        }
    }

    draw();
}

}  // namespace militia

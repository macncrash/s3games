#include "game/battery.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace battery {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 8;
constexpr float kHorizon = 64.f;
constexpr float kSpan = 160.f;
constexpr float kZNear = 10.5f;
constexpr float kPpm = 19.f;
constexpr float kRoadHalf = 6.f;
constexpr float kGate = 16.f;
constexpr float kSpeed = 4.15f;
constexpr float kSpacing = 7.6f;
constexpr float kLeadZ = 38.f;
constexpr float kReload = 0.42f;
constexpr float kBlast = 2.7f;
constexpr float kSector = 52.f;
constexpr float kFA = 0.18f;
constexpr float kFB = 0.0072f;
constexpr float kFMax = 0.85f;
constexpr float kSlewX = 8.f;
constexpr float kSlewZ = 36.f;
constexpr float kPi = 3.14159265f;

// Quiet beside the guns, a bend farther up the valley, so the lip stays centred.
float bend(float z) {
    if (z < 0.f) z = 0.f;
    float fade = z / (z + 30.f);
    return std::sin(z * 0.055f) * 7.5f * fade;
}

float tallOf(int kind, bool dead) {
    if (dead) return 2.7f;
    static const float t[4] = {3.4f, 4.4f, 4.6f, 3.8f};
    if (kind < 0 || kind > 3) kind = 1;
    return t[kind];
}

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Fight || mode_ == Mode::Pause) return 1;
    return 0;
}

float Game::flightAt(float z) const { return std::clamp(kFA + kFB * z, kFA, kFMax); }

float Game::predictZ(float z, float speed) const {
    float zh = (z - speed * kFA) / (1.f + speed * kFB);
    float f = kFA + kFB * zh;
    if (f > kFMax) zh = z - speed * kFMax;
    else if (f < kFA) zh = z - speed * kFA;
    return zh;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float f = std::clamp((0.16f - t) / 0.16f, 0.f, 1.f);
    return int(f * 11.f);
}

int Game::incoming(int index) const {
    int n = 0;
    for (const Shell& s : shells_)
        if (s.target == index && s.t > 0.f) ++n;
    return n;
}

Game::Proj Game::project(float wx, float wz) const {
    Proj p;
    if (!(wz > kZNear + 0.15f)) return p;
    float t = kZNear / wz;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * kSpan;
    p.x = 160.f + (wx - bend(0.f)) * p.ppm;
    p.ok = p.y > kHorizon - 6.f && p.y < gs::SCREEN_H + 40.f;
    return p;
}

void Game::buildProps() {
    props_.clear();
    for (int i = 0; i < 16; ++i) {
        float z = 18.f + float(i / 2) * 9.5f;
        float sign = (i & 1) ? 1.f : -1.f;
        float lat = sign * (kRoadHalf + 2.6f + float((i * 3) % 5) * 0.35f);
        props_.push_back({z, lat, (i % 3 == 0) ? 1 : 0});
    }
    props_.push_back({kGate, kRoadHalf - 0.3f, 2});
    props_.push_back({kGate, -(kRoadHalf - 0.3f), 2});
}

void Game::spawnColumn() {
    veh_.assign(kColumn, {});
    const int pat[kColumn] = {0, 1, 1, 2, 1, 1, 1, 3};
    const int hp[4] = {1, 1, 1, 2};
    for (int i = 0; i < kColumn; ++i) {
        Veh& v = veh_[i];
        v.kind = pat[i];
        v.hp = hp[v.kind];
        v.z = kLeadZ + float(i) * kSpacing;
        v.lat = (i & 1) ? 0.62f : -0.48f;
        v.speed = kSpeed;
        v.flash = 0;
        v.smoke = 0.08f * float(i);
        v.dead = false;
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    stopped_ = 0;
    through_ = 0;
    shots_ = 0;
    t_ = 0;
    hold_ = 0;
    reload_ = 0;
    shake_ = 0;
    failFlash_ = 0;
    fanStep_ = -1;
    gunKick_[0] = gunKick_[1] = 0;
    shells_.clear();
    booms_.clear();
    puffs_.clear();
    spawnColumn();
    aimLat_ = 0;
    aimZ_ = kLeadZ;
}

void Game::beginFight() {
    bootTitle();
    mode_ = Mode::Fight;
    t_ = 0;
    reload_ = 0.22f;
    blip(true);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    bootTitle();
    if (bot_) beginFight();
}

void Game::blip(bool high) {
    sys_->apu.tone(2, high ? 880.f : 480.f, 0.05f);
    blip_ = 0.05f;
}

void Game::fanfare(bool good) {
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = good;
}

void Game::shoot(int target) {
    if (reload_ > 0.f || mode_ != Mode::Fight) return;
    reload_ = kReload;
    Shell s;
    s.z = aimZ_;
    s.x = bend(aimZ_) + aimLat_;
    s.flight = flightAt(aimZ_);
    s.t = s.flight;
    s.target = target;
    s.gun = shots_ & 1;
    shells_.push_back(s);
    gunKick_[s.gun] = 1.f;
    ++shots_;
    shake_ = std::max(shake_, 0.3f);
    sys_->apu.noiseBurst(0.3f, 860.f, 0.08f);
    sys_->apu.tone(1, 86.f, 0.08f);
    shotTone_ = 0.07f;
    sys_->rumble(0.25f, 0.5f, 45);
}

void Game::hit(Veh& v) {
    if (v.dead) return;
    --v.hp;
    v.flash = 0.16f;
    if (v.hp > 0) return;
    v.hp = 0;
    v.dead = true;
    v.speed = 0;
    ++stopped_;
}

void Game::explode(const Shell& s) {
    booms_.push_back({s.x, s.z, 0.f, 0.55f});
    if ((int)puffs_.size() < 30) {
        puffs_.push_back({s.x - 0.3f, s.z, 0.f, 0.f, 0.65f});
        puffs_.push_back({s.x + 0.45f, s.z + 0.2f, 0.f, 3.f, 0.55f});
    }
    shake_ = std::max(shake_, 0.75f);
    sys_->apu.noiseBurst(0.52f, 180.f, 0.18f);
    sys_->rumble(0.5f, 0.8f, 90);
    const float r2 = kBlast * kBlast;
    for (Veh& v : veh_) {
        if (v.dead) continue;
        float dx = (bend(v.z) + v.lat) - s.x;
        float dz = v.z - s.z;
        if (dx * dx + dz * dz <= r2) hit(v);
    }
}

void Game::win() {
    if (mode_ == Mode::Victory) return;
    mode_ = Mode::Victory;
    won_ = true;
    hold_ = 0.45f;
    fanfare(true);
    sys_->rumble(0.3f, 0.55f, 160);
}

void Game::lose() {
    if (mode_ != Mode::Fight) return;
    mode_ = Mode::Fail;
    won_ = false;
    hold_ = 0.45f;
    failFlash_ = 0.4f;
    shake_ = 1.f;
    fanfare(false);
    sys_->rumble(0.8f, 0.4f, 180);
}

void Game::botAct() {
    int pick = -1;
    float best = 1.e9f;
    float zh = 0;
    for (int i = 0; i < (int)veh_.size(); ++i) {
        const Veh& v = veh_[i];
        if (v.dead || v.z > kSector) continue;
        if (incoming(i) >= v.hp) continue;
        float pz = predictZ(v.z, v.speed);
        if (pz < kGate + 0.8f) continue;
        if (v.z < best) {
            best = v.z;
            pick = i;
            zh = pz;
        }
    }
    if (pick < 0) return;
    aimZ_ = zh;
    aimLat_ = veh_[pick].lat;
    shoot(pick);
}

void Game::humanAct() {
    const gs::Pad& pad = sys_->pad;
    float ix = (pad.down(gs::BTN_RIGHT) ? 1.f : 0.f) - (pad.down(gs::BTN_LEFT) ? 1.f : 0.f);
    float iy = (pad.down(gs::BTN_UP) ? 1.f : 0.f) - (pad.down(gs::BTN_DOWN) ? 1.f : 0.f);
    if (std::fabs(pad.axisX) > 0.22f && ix == 0.f) ix = pad.axisX;
    float mag = std::sqrt(ix * ix + iy * iy);
    if (mag > 1.f) {
        ix /= mag;
        iy /= mag;
    }
    aimLat_ = std::clamp(aimLat_ + ix * kSlewX * kDt, -5.2f, 5.2f);
    aimZ_ = std::clamp(aimZ_ + iy * kSlewZ * kDt, kGate + 1.f, 120.f);
    bool fire = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO) || pad.accel > 0.45f;
    if (fire) shoot(-1);
}

void Game::update() {
    if (mode_ != Mode::Fight) return;
    t_ += kDt;
    if (reload_ > 0.f) reload_ -= kDt;
    for (Veh& v : veh_) {
        if (v.dead) continue;
        v.z -= v.speed * kDt;
    }
    for (int i = 0; i < (int)shells_.size();) {
        shells_[i].t -= kDt;
        if (shells_[i].t > 0.f) {
            ++i;
            continue;
        }
        explode(shells_[i]);
        shells_.erase(shells_.begin() + i);
    }
    if (mode_ != Mode::Fight) return;
    int crossed = 0;
    for (const Veh& v : veh_)
        if (!v.dead && v.z <= kGate) ++crossed;
    if (crossed) {
        through_ += crossed;
        lose();
        return;
    }
    bool any = false;
    for (const Veh& v : veh_)
        if (!v.dead) any = true;
    if (!any) {
        win();
        return;
    }
    if (bot_) botAct();
    else humanAct();
}

void Game::tickFx() {
    shake_ = std::max(0.f, shake_ - kDt * 1.5f);
    failFlash_ = std::max(0.f, failFlash_ - kDt);
    for (float& k : gunKick_) k = std::max(0.f, k - kDt * 3.4f);
    for (int i = 0; i < (int)booms_.size();) {
        booms_[i].age += kDt;
        if (booms_[i].age >= booms_[i].life) booms_.erase(booms_.begin() + i);
        else ++i;
    }
    for (int i = 0; i < (int)puffs_.size();) {
        puffs_[i].age += kDt;
        puffs_[i].hop += 20.f * kDt;
        if (puffs_[i].age >= puffs_[i].life) puffs_.erase(puffs_.begin() + i);
        else ++i;
    }
    for (Veh& v : veh_) {
        if (v.flash > 0.f) v.flash -= kDt;
        if (!v.dead) continue;
        if (mode_ != Mode::Fight && mode_ != Mode::Victory) continue;
        v.smoke -= kDt;
        if (v.smoke <= 0.f && (int)puffs_.size() < 30) {
            v.smoke = 0.24f;
            puffs_.push_back({bend(v.z) + v.lat, v.z, 0.f, 0.f, 0.8f});
        }
    }
}

void Game::serviceAudio() {
    if (shotTone_ > 0.f) {
        shotTone_ -= kDt;
        if (shotTone_ <= 0.f) sys_->apu.tone(1, 0.f, 0.f);
    }
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.12f) {
            static const float good[] = {392.f, 523.f, 659.f, 784.f};
            static const float bad[] = {196.f, 155.f, 123.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(0, 98.f, 0.028f);
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
    }
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) {
            width += 10.f * scale;
            continue;
        }
        width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw;
    }
}

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    v.setFogColor(failFlash_ > 0.f ? gs::rgb4(12, 3, 2) : gs::rgb4(11, 9, 7));
    const float origin = bend(0.f);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y <= int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int r = std::clamp(int(3.f + u * 8.f), 0, 15);
            int g = std::clamp(int(5.f + u * 4.f), 0, 15);
            int b = std::clamp(int(12.f - u * 5.f), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) - kHorizon) / kSpan;
        if (t < 0.004f) t = 0.004f;
        float wz = kZNear / t;
        float ppm = kPpm * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + (bend(wz) - origin) * ppm + shx;
        rd.hw = std::max(2.f, kRoadHalf * ppm);
        rd.v = wz * 36.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz / 3.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.16f - t) / 0.16f, 0.f, 1.f);
        int fog = int(fogT * 12.f);
        if (failFlash_ > 0.f) fog = std::min(16, fog + int(failFlash_ * 10.f));
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = gs::rgb4(2, 4, 2);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0;
    if (shake_ > 0.f) shx = std::sin(float(sys_->frame) * 1.4f) * 4.2f * std::min(shake_, 1.f);
    road(shx);

    if (mode_ == Mode::Title) text("BATTERY", 160.f + shx, 22.f, 1.5f, PAL_GOLD);
    else if (mode_ == Mode::Victory) text("COLUMN STOPS", 160.f + shx, 148.f, 1.2f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text("COLUMN PASSES", 160.f + shx, 148.f, 1.1f, PAL_ALERT);

    const bool showSight = mode_ == Mode::Title || mode_ == Mode::Fight || mode_ == Mode::Pause;
    if (showSight) {
        Proj p = project(bend(aimZ_) + aimLat_, aimZ_);
        if (p.ok) spr(art_.sight, p.x + shx, p.y, 18.f, PAL_FX, false, 0, false, false);
    }

    for (const Shell& s : shells_) {
        float u = 1.f - s.t / std::max(0.05f, s.flight);
        u = std::clamp(u, 0.f, 1.f);
        float mx = (s.gun ? 250.f : 72.f) + shx;
        float my = 164.f + gunKick_[s.gun] * 6.f;
        Proj p = project(s.x, s.z);
        if (!p.ok) continue;
        float x = mx + (p.x + shx - mx) * u;
        float y = my + (p.y - my) * u - std::sin(u * kPi) * 34.f;
        spr(art_.shell, x, y, 9.f, PAL_FX, false, 0, false, false);
    }
    if (gunKick_[0] > 0.45f) spr(art_.shell, 72.f + shx, 156.f, 12.f, PAL_FX, false, 0, false, false);
    if (gunKick_[1] > 0.45f) spr(art_.shell, 250.f + shx, 156.f, 12.f, PAL_FX, false, 0, false, false);

    const float bagY = 224.f;
    const float bagX[4] = {46.f, 124.f, 202.f, 280.f};
    for (float x : bagX) spr(art_.bags, x + shx, bagY, 30.f, PAL_GUN, false, 0, true, false);
    spr(art_.gun, 72.f + shx, 214.f + gunKick_[0] * 6.f, 58.f, PAL_GUN, false, 0, true, false);
    spr(art_.gun, 250.f + shx, 214.f + gunKick_[1] * 6.f, 58.f, PAL_GUN, false, 0, true, false);
    spr(art_.crew, 38.f + shx, 206.f, 24.f, PAL_GUN, false, 0, true, false);
    spr(art_.crew, 286.f + shx, 206.f, 24.f, PAL_GUN, true, 0, true, false);

    struct Item {
        float z;
        int ord;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(64);
    for (int i = 0; i < (int)booms_.size(); ++i) items.push_back({booms_[i].z, 0, 0, i});
    for (int i = 0; i < (int)shells_.size(); ++i) items.push_back({shells_[i].z, 1, 1, i});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({puffs_[i].z, 2, 2, i});
    for (int i = 0; i < (int)veh_.size(); ++i) items.push_back({veh_[i].z, 3, 3, i});
    for (int i = 0; i < (int)props_.size(); ++i) items.push_back({props_[i].z, props_[i].kind == 2 ? 4 : 6, 4, i});
    items.push_back({kGate, 7, 5, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.z != b.z) return a.z < b.z;
        return a.ord < b.ord;
    });

    for (const Item& it : items) {
        if (it.kind == 0) {
            const Boom& b = booms_[it.id];
            Proj p = project(b.x, b.z);
            if (!p.ok) continue;
            float u = b.age / std::max(0.05f, b.life);
            float h = std::clamp(p.ppm * (2.6f + u * 6.f), 6.f, 72.f);
            spr(art_.boom, p.x + shx, p.y - h * 0.25f, h, PAL_FX, false, fogFor(b.z), false, false);
        } else if (it.kind == 1) {
            const Shell& s = shells_[it.id];
            Proj p = project(s.x, s.z);
            if (!p.ok) continue;
            spr(art_.sight, p.x + shx, p.y, std::clamp(p.ppm * 1.5f, 8.f, 16.f), PAL_FX, false, fogFor(s.z), false, false);
        } else if (it.kind == 2) {
            const Puff& f = puffs_[it.id];
            Proj p = project(f.x, f.z);
            if (!p.ok) continue;
            float h = std::clamp(p.ppm * 2.1f, 5.f, 36.f);
            spr(art_.smoke, p.x + shx, p.y - f.hop - h * 0.4f, h, PAL_FX, false, fogFor(f.z), false, false);
        } else if (it.kind == 3) {
            const Veh& vh = veh_[it.id];
            float wx = bend(vh.z) + vh.lat;
            Proj p = project(wx, vh.z);
            if (!p.ok) continue;
            int fog = fogFor(vh.z);
            const gs::Mipped* body = &art_.truck;
            int pal = PAL_TRUCK;
            if (vh.dead) {
                body = &art_.wreck;
                pal = PAL_FX;
            } else if (vh.kind == 0) {
                body = &art_.car;
                pal = PAL_CAR;
            } else if (vh.kind == 2) {
                body = &art_.tanker;
                pal = PAL_TRUCK;
            } else if (vh.kind == 3) {
                body = &art_.armor;
                pal = PAL_ARMOR;
            }
            float h = std::clamp(tallOf(vh.kind, vh.dead) * p.ppm, 3.f, 78.f);
            if (vh.flash > 0.f) spr(art_.boom, p.x + shx, p.y - h * 0.45f, h * 0.55f, PAL_FX, false, fog, false, false);
            spr(art_.shadow, p.x + shx, p.y, h * 0.32f, PAL_FX, false, 0, false, true);
            spr(*body, p.x + shx, p.y, h, pal, false, fog, true, false);
        } else if (it.kind == 4) {
            const Prop& pr = props_[it.id];
            Proj p = project(bend(pr.z) + pr.lat, pr.z);
            if (!p.ok) continue;
            int fog = fogFor(pr.z);
            if (pr.kind == 2) {
                float h = std::clamp(3.6f * p.ppm, 6.f, 70.f);
                spr(art_.post, p.x + shx, p.y, h, PAL_POST, pr.lat < 0, fog, true, false);
            } else {
                float world = pr.kind == 1 ? 7.2f : 6.3f;
                float h = std::clamp(world * p.ppm, 4.f, 92.f);
                const gs::Mipped& m = pr.kind == 1 ? art_.pine : art_.tree;
                spr(m, p.x + shx, p.y, h, PAL_TREE, pr.lat < 0, fog, true, false);
            }
        } else if (it.kind == 5) {
            Proj p = project(bend(kGate), kGate);
            if (!p.ok) continue;
            float w = (kRoadHalf * 2.f - 0.8f) * p.ppm;
            float h = std::clamp(p.ppm * 0.42f, 3.f, 10.f);
            sprBox(art_.tape, p.x + shx, p.y, w, h, PAL_POST, fogFor(kGate), false);
        }
    }

    float anim = float(sys_->frame) * kDt;
    spr(art_.cloud, 78.f + std::sin(anim * 0.25f) * 10.f, 26.f, 16.f, PAL_FX, false, 0, false, false);
    spr(art_.cloud, 214.f + std::sin(anim * 0.18f + 1.f) * 12.f, 18.f, 13.f, PAL_FX, true, 0, false, false);
    spr(art_.sun, 292.f, 16.f, 15.f, PAL_FX, false, 0, false, false);

    if (mode_ == Mode::Title) {
        hudC(25, "THE ROAD BELOW", PAL_GOLD);
        hudC(26, "STOP THE COLUMN", PAL_ALERT);
        hudC(27, "ARROWS AIM   C FIRE   START", PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "HOLD FIRE", PAL_GOLD);
        hudC(27, "START RESUMES", PAL_TEXT);
    } else {
        char buf[40];
        std::snprintf(buf, sizeof buf, "STOPPED %d/%d", stopped_, kColumn);
        hud(1, 0, buf, mode_ == Mode::Victory ? PAL_GOOD : PAL_TEXT);
        if (mode_ == Mode::Fail) hudC(26, "THE COLUMN PASSES", PAL_ALERT);
        else if (mode_ == Mode::Victory) hudC(26, "THE COLUMN STOPS", PAL_GOOD);
        else if (mode_ == Mode::Fight && t_ < 4.f) hudC(27, "DOWN LEADS   C FIRES", PAL_GOLD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        float a = float(sys.frame) * kDt;
        aimZ_ = 48.f + std::sin(a * 0.75f) * 14.f;
        aimLat_ = std::sin(a * 0.4f) * 1.1f;
        if (pad.pressed(gs::BTN_START)) beginFight();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Fight) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Fight;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) beginFight();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    if (mode_ != Mode::Pause) tickFx();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Fail) sys.setLight(210, 36, 28);
    else if (mode_ == Mode::Fight) sys.setLight(170, 120, 36);
    else sys.setLight(50, 80, 140);
}

}  // namespace battery

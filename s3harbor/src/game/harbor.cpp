#include "game/harbor.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace harbor {
namespace {

constexpr float kHor = 76.f;
constexpr float kBoatY = 188.f;
constexpr float kNear = 30.f;
constexpr float kCruise = 0.50f;
constexpr float kFast = 0.72f;
constexpr float kSlow = 0.28f;
constexpr float kSee = 76.f;
constexpr float kSalvo2 = 34.f;
constexpr float kGunFar = 46.f;
constexpr float kGunNear = 14.f;
constexpr float kTravel = 2.55f;
constexpr float kHit = 0.20f;
constexpr float kMouth = 780.f;
constexpr int kMag = 8;
constexpr int kHull = 5;
constexpr int kShotLife = 10;
constexpr int kN = 6;

struct Spec {
    float z;
    int side;
};
constexpr Spec kSpec[kN] = {
    {150.f, -1}, {250.f, 1}, {350.f, -1}, {450.f, 1}, {550.f, -1}, {650.f, 1},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.72f);
    resetWorld();
    mode_ = bot_ ? Mode::Run : Mode::Title;
}

void Game::resetWorld() {
    for (int i = 0; i < kN; i++) {
        fort_[i] = Fort{};
        fort_[i].z = kSpec[i].z;
        fort_[i].side = kSpec[i].side;
        fort_[i].alive = true;
    }
    shots_.clear();
    incoming_.clear();
    fx_.clear();
    graves_.clear();
    mag_ = kMag;
    hull_ = kHull;
    silenced_ = 0;
    cool_ = 0;
    inv_ = 0;
    note_ = 0;
    fan_ = 0;
    muzzle_ = 0;
    boatZ_ = 0;
    boatX_ = 0;
    vx_ = 0;
    speed_ = kCruise;
    shake_ = 0;
    paused_ = false;
    over_ = false;
    won_ = false;
    cause_.clear();
    if (sys_) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        auto& p = sys.pad;
        if (!bot_ && (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO))) {
            resetWorld();
            mode_ = Mode::Run;
        }
        draw();
        mix();
        return;
    }
    if (mode_ == Mode::Run) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) paused_ = !paused_;
        if (!paused_) update();
    } else if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || sys.pad.pressed(gs::BTN_TURBO))) {
        resetWorld();
        mode_ = Mode::Run;
    }
    draw();
    mix();
}

void Game::update() {
    bool left = false, right = false, up = false, down = false;
    int fire = 99;
    if (bot_) botAct(left, right, fire);
    else {
        auto& p = sys_->pad;
        left = p.down(gs::BTN_LEFT);
        right = p.down(gs::BTN_RIGHT);
        up = p.down(gs::BTN_UP);
        down = p.down(gs::BTN_DOWN);
        if (p.pressed(gs::BTN_A)) fire = -1;
        else if (p.pressed(gs::BTN_C)) fire = 1;
        else if (p.pressed(gs::BTN_B) || p.pressed(gs::BTN_TURBO)) fire = 0;
    }
    move(left, right, up, down);
    tickShots();
    if (mode_ != Mode::Run) return;
    salvos();
    tickIncoming();
    if (mode_ != Mode::Run) return;
    if (fire != 99) trigger(fire);
    rules();
    tickFx();
}

void Game::botAct(bool& left, bool& right, int& fire) {
    const Hostile* threat = nullptr;
    for (const auto& h : incoming_) {
        if (h.life <= 0) continue;
        if (!threat || h.life < threat->life) threat = &h;
    }
    float goal = 0.f;
    if (threat) {
        float lock = threat->lat1;
        float dir = (0.92f - lock) >= (lock + 0.92f) ? 1.f : -1.f;
        goal = clampf(lock + dir * 0.72f, -0.92f, 0.92f);
    }
    if (boatX_ < goal - 0.02f) right = true;
    else if (boatX_ > goal + 0.02f) left = true;

    if (cool_ > 0 || mag_ <= 0) return;
    if (threat && threat->life <= 6 && std::fabs(boatX_ - threat->lat1) < kHit + 0.06f) return;
    int id = target(0);
    if (id >= 0) fire = fort_[id].side;
}

void Game::move(bool left, bool right, bool up, bool down) {
    float ax = 0.f;
    if (left && !right) ax -= 0.009f;
    if (right && !left) ax += 0.009f;
    if (ax == 0.f) vx_ *= 0.84f;
    else vx_ += ax;
    vx_ = clampf(vx_, -0.042f, 0.042f);
    boatX_ = clampf(boatX_ + vx_, -0.92f, 0.92f);
    speed_ = kCruise;
    if (up && !down) speed_ = kFast;
    if (down && !up) speed_ = kSlow;
    boatZ_ += speed_;
}

int Game::target(int side) const {
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < kN; i++) {
        const Fort& f = fort_[i];
        if (!f.alive || f.doomed) continue;
        if (side != 0 && f.side != side) continue;
        float d = f.z - boatZ_;
        if (d < kGunNear || d > kGunFar) continue;
        if (d < bestD) {
            bestD = d;
            best = i;
        }
    }
    return best;
}

bool Game::inbound() const {
    for (const auto& s : shots_)
        if (s.kill && s.life > 0) return true;
    return false;
}

void Game::trigger(int side) {
    if (mode_ != Mode::Run || cool_ > 0 || mag_ <= 0) return;
    int id = target(side);
    int use = side;
    if (side == 0) use = id >= 0 ? fort_[id].side : (boatX_ >= 0 ? 1 : -1);
    mag_--;
    cool_ = 14;
    muzzle_ = 5;
    if (id >= 0) {
        fort_[id].doomed = true;
        Shot s;
        s.fort = id;
        s.lat0 = boatX_;
        s.lat1 = float(fort_[id].side) * 1.16f;
        s.life = s.life0 = kShotLife;
        s.kill = true;
        shots_.push_back(s);
        sys_->apu.noiseBurst(0.28f, 1600.f, 0.07f);
        blip(180.f, 0.1f, 5);
    } else {
        Shot s;
        s.fort = -1;
        s.lat0 = boatX_;
        s.lat1 = float(use) * 1.16f;
        s.dEnd = 22.f;
        s.life = s.life0 = 12;
        s.kill = false;
        shots_.push_back(s);
        sys_->apu.noiseBurst(0.12f, 900.f, 0.05f);
        blip(120.f, 0.06f, 4);
    }
}

void Game::tickShots() {
    std::vector<Shot> keep;
    keep.reserve(shots_.size());
    for (auto s : shots_) {
        s.life--;
        if (s.life > 0) {
            keep.push_back(s);
            continue;
        }
        if (s.kill && s.fort >= 0) killFort(s.fort);
        else burst(s.dEnd, s.lat1, 0);
    }
    shots_.swap(keep);
}

void Game::salvos() {
    for (int i = 0; i < kN; i++) {
        Fort& f = fort_[i];
        if (!f.alive) continue;
        float d = f.z - boatZ_;
        bool shoot = false;
        if (!f.salvo1 && d <= kSee && d > kGunNear) {
            f.salvo1 = true;
            shoot = true;
        } else if (!f.salvo2 && d <= kSalvo2 && d > kGunNear) {
            f.salvo2 = true;
            shoot = true;
        }
        if (!shoot) continue;
        Hostile h;
        h.z0 = f.z;
        h.lat0 = float(f.side) * 1.16f;
        h.lat1 = boatX_;
        h.side = f.side;
        h.life0 = std::max(8, int(std::lround(d / kTravel)));
        h.life = h.life0;
        incoming_.push_back(h);
        f.flash = 7;
        burst(d - 2.f, float(f.side) * 1.02f, 3);
        sys_->apu.noiseBurst(0.2f, 500.f, 0.1f);
        blip(90.f, 0.08f, 6);
    }
}

void Game::tickIncoming() {
    std::vector<Hostile> keep;
    keep.reserve(incoming_.size());
    for (auto h : incoming_) {
        h.life--;
        if (h.life > 0) {
            keep.push_back(h);
            continue;
        }
        if (std::fabs(boatX_ - h.lat1) < kHit) hole();
        else burst(0.6f, h.lat1, 0);
        if (mode_ != Mode::Run) {
            incoming_.clear();
            return;
        }
    }
    incoming_.swap(keep);
}

void Game::rules() {
    if (mode_ != Mode::Run) return;
    if (hull_ <= 0) {
        lose("HULL GONE");
        return;
    }
    bool any = false;
    for (const auto& f : fort_)
        if (f.alive) any = true;
    if (mag_ <= 0 && !inbound() && any) {
        lose("MAGAZINE EMPTY");
        return;
    }
    for (const auto& f : fort_) {
        if (!f.alive || f.doomed) continue;
        if (f.z - boatZ_ < kGunNear) {
            lose("FORT STILL STANDS");
            return;
        }
    }
    if (!any && boatZ_ >= kMouth && hull_ > 0) win();
}

void Game::tickFx() {
    if (cool_ > 0) cool_--;
    if (inv_ > 0) inv_--;
    if (muzzle_ > 0) muzzle_--;
    shake_ *= 0.82f;
    for (auto& f : fort_)
        if (f.flash > 0) f.flash--;
    std::vector<Fx> fkeep;
    for (auto f : fx_) {
        f.life--;
        if (f.life > 0) fkeep.push_back(f);
    }
    fx_.swap(fkeep);
    std::vector<Grave> gkeep;
    for (auto g : graves_) {
        g.life--;
        if (g.life > 0) gkeep.push_back(g);
    }
    graves_.swap(gkeep);
}

void Game::killFort(int i) {
    if (i < 0 || i >= kN || !fort_[i].alive) return;
    fort_[i].alive = false;
    silenced_++;
    Grave g;
    g.z = fort_[i].z;
    g.side = fort_[i].side;
    g.life = 80;
    graves_.push_back(g);
    burst(fort_[i].z - boatZ_, float(fort_[i].side) * 1.16f, 1);
    burst(fort_[i].z - boatZ_ - 1.f, float(fort_[i].side) * 1.05f, 2);
    sys_->apu.noiseBurst(0.4f, 320.f, 0.18f);
    blip(320.f, 0.12f, 8);
    sys_->rumble(0.4f, 0.2f, 80);
}

void Game::hole() {
    if (inv_ > 0 || mode_ != Mode::Run) return;
    hull_--;
    inv_ = 28;
    shake_ = 7.f;
    burst(1.2f, boatX_, 1);
    sys_->apu.noiseBurst(0.48f, 180.f, 0.22f);
    blip(70.f, 0.12f, 8);
    sys_->rumble(0.8f, 0.5f, 140);
    if (hull_ <= 0) lose("HULL GONE");
}

void Game::win() {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    cause_ = "CHANNEL CLEAR";
    fan_ = 0;
    sys_->rumble(0.2f, 0.5f, 160);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    cause_ = why;
    fan_ = 0;
}

void Game::blip(float freq, float vol, int hold) {
    sys_->apu.tone(0, freq, vol);
    note_ = hold;
}

void Game::burst(float z, float lat, int kind) {
    if (fx_.size() > 48) return;
    Fx f;
    f.z = boatZ_ + z;
    f.lat = lat;
    f.kind = kind;
    f.life0 = kind == 3 ? 5 : (kind == 1 ? 16 : 14);
    f.life = f.life0;
    fx_.push_back(f);
}

void Game::mix() {
    auto& a = sys_->apu;
    if (mode_ == Mode::Run && !paused_) a.tone(2, speed_ > 0.6f ? 52.f : 40.f, 0.028f);
    else a.tone(2, 0.f, 0.f);
    if (mode_ == Mode::Won) {
        static const float n[] = {349.2f, 440.f, 523.3f, 698.5f, 880.f};
        int s = fan_ / 8;
        if (fan_ < 80 && s < 5 && fan_ % 8 == 0) a.tone(0, n[s], 0.11f);
        else if (fan_ == 56) a.tone(0, 0, 0);
        if (fan_ < 200) fan_++;
    } else if (mode_ == Mode::Lost) {
        static const float n[] = {196.f, 146.8f, 110.f, 82.4f};
        int s = fan_ / 10;
        if (fan_ < 70 && s < 4 && fan_ % 10 == 0) a.tone(0, n[s], 0.1f);
        else if (fan_ == 48) a.tone(0, 0, 0);
        if (fan_ < 200) fan_++;
    } else if (note_ > 0) {
        if (--note_ == 0) a.tone(0, 0, 0);
    }
}

float Game::halfAt(float y) const { return 11.f + (y - kHor) * 0.98f; }

float Game::dForY(float y) const {
    float k = (y - kHor) / (kBoatY - kHor);
    if (k < 0.04f) return 260.f;
    return kNear * (1.f - k) / k;
}

Game::Proj Game::project(float d, float lat) const {
    Proj p;
    if (d < -20.f || d > 280.f) return p;
    float k = kNear / (d + kNear);
    float y = kHor + (kBoatY - kHor) * k;
    p.y = y + oy_;
    p.x = 160.f + lat * halfAt(y) + ox_;
    p.s = clampf(0.2f + k, 0.15f, 1.7f);
    p.ok = p.y > -48.f && p.y < gs::SCREEN_H + 48.f;
    return p;
}

int Game::fogFor(float d) const {
    if (d > 150.f) return 11;
    if (d > 100.f) return 7;
    if (d > 60.f) return 3;
    return 0;
}

void Game::spr(const gs::Image& img, float cx, float cy, float dh, int pal, bool flip, int fog, bool shadow) {
    if (img.w == 0 || img.h == 0 || dh < 1.f) return;
    float sc = dh / float(img.h);
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::max(1, int(std::lround(float(img.w) * sc))));
    s.h = int16_t(std::max(1, int(std::lround(dh))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float dh, int pal, bool flip, int fog, bool shadow) {
    if (m.h <= 0) return;
    spr(m.pick(dh), cx, cy, dh, pal, flip, fog, shadow);
}

void Game::text(const std::string& s, float x, float y, int pal, float scale) {
    float h = float(art_.cellH) * scale;
    float a = float(art_.cellW) * scale;
    for (unsigned char ch : s) {
        if (ch != ' ' && ch < 128 && art_.glyph[ch].w) spr(art_.glyph[ch], x + 7.f * scale, y + h * 0.5f, h, pal);
        x += a;
    }
}

void Game::textC(const std::string& s, float y, int pal, float scale) {
    float w = float(s.size()) * float(art_.cellW) * scale;
    text(s, 160.f - w * 0.5f, y, pal, scale);
}

void Game::plate(float x, float y, float w, float h) {
    if (art_.plate.w == 0 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    s.img = art_.plate;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = PAL_DEAD;
    sys_->vdp.sprite(s);
}

void Game::skyRoad() {
    ox_ = std::sin(float(sys_->frame) * 1.7f) * shake_;
    oy_ = std::cos(float(sys_->frame) * 2.1f) * shake_ * 0.35f;
    float scroll = mode_ == Mode::Title ? float(sys_->frame) * 0.35f : boatZ_;
    auto& v = sys_->vdp;
    v.roadTime = int(sys_->frame);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = clampf((float(y) + 8.f) / (kHor + 8.f), 0.f, 1.f);
        int r = int(1 + 9.f * t);
        int g = int(2 + 6.f * t);
        int b = int(7 - 2.f * t);
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
        if (y < int(kHor)) {
            v.road[y].on = false;
            v.lineFog[y] = 0;
            continue;
        }
        float d = dForY(float(y));
        gs::RoadLine& ln = v.road[y];
        ln.on = true;
        ln.cx = 160.f + ox_;
        ln.hw = std::max(8.f, halfAt(float(y)));
        ln.v = (scroll + d) * 26.f;
        ln.pal = PAL_ROAD;
        ln.style = 2;
        ln.band = uint8_t((int(std::floor(scroll + d)) / 5) & 1);
        ln.left = 0;
        ln.right = 0;
        int fog = 0;
        if (d > 160.f) fog = 10;
        else if (d > 90.f) fog = 5;
        v.lineFog[y] = uint8_t(fog);
    }
}

void Game::hud() {
    char buf[48];
    std::snprintf(buf, sizeof buf, "HULL %d", hull_);
    text(buf, 8, 4, hull_ <= 1 ? PAL_RED : PAL_TEXT, 1);
    std::snprintf(buf, sizeof buf, "LEFT %d", kN - silenced_);
    text(buf, 210, 4, PAL_DIM, 1);
    for (int i = 0; i < kMag; i++) {
        float x = 78.f + float(i) * 12.f;
        spr(i < mag_ ? art_.hot : art_.shell, x, 11, 12, i < mag_ ? PAL_SHOT : PAL_DEAD);
    }
    int side = 0;
    int id = target(0);
    if (id >= 0) side = fort_[id].side;
    if (paused_) textC("PAUSED", 96, PAL_AMBER, 2);
    else if (side < 0) textC("PORT IN RANGE", 24, PAL_AMBER, 1);
    else if (side > 0) textC("STARBOARD IN RANGE", 24, PAL_AMBER, 1);
    else if (mag_ <= 2 && silenced_ < kN && mode_ == Mode::Run) textC("MAGAZINE LOW", 24, PAL_RED, 1);
}

void Game::combatSprites() {
    if (muzzle_ > 0) {
        Proj m = project(3.f, boatX_);
        if (m.ok) spr(art_.flash, m.x, m.y - 10.f * m.s, 16.f * m.s, PAL_FX);
    }
    for (const auto& s : shots_) {
        float u = 1.f - float(s.life) / float(std::max(1, s.life0));
        float dFort = s.kill && s.fort >= 0 ? fort_[s.fort].z - boatZ_ : s.dEnd;
        float d = std::max(0.f, dFort) * u;
        float lat = s.lat0 + (s.lat1 - s.lat0) * u;
        Proj p = project(d, lat);
        if (!p.ok) continue;
        spr(art_.shell, p.x, p.y, std::max(6.f, 12.f * p.s), PAL_SHOT);
    }
    for (const auto& h : incoming_) {
        float u = 1.f - float(h.life) / float(std::max(1, h.life0));
        float d = std::max(0.f, (h.z0 - boatZ_) * (1.f - u));
        float lat = h.lat0 + (h.lat1 - h.lat0) * u;
        Proj p = project(d, lat);
        if (p.ok) spr(art_.hot, p.x, p.y, std::max(6.f, 12.f * p.s), PAL_SHOT, false, fogFor(d));
        Proj mark = project(1.4f, h.lat1);
        if (mark.ok) spr(art_.hot, mark.x, mark.y, 7.f + float(sys_->frame % 6), PAL_SHOT);
    }
    for (const auto& f : fx_) {
        float d = f.z - boatZ_;
        Proj p = project(d, f.lat);
        if (!p.ok) continue;
        float u = float(f.life) / float(std::max(1, f.life0));
        if (f.kind == 0) spr(art_.splash, p.x, p.y, (8.f + 18.f * (1.f - u)) * p.s, PAL_FX, false, fogFor(d));
        else if (f.kind == 1) spr(art_.burst, p.x, p.y, (16.f + 22.f * u) * std::max(p.s, 0.4f), PAL_FX);
        else if (f.kind == 2) spr(art_.puff, p.x, p.y, (10.f + 14.f * u) * p.s, PAL_FX, false, 2);
        else spr(art_.flash, p.x, p.y, 14.f * std::max(p.s, 0.35f), PAL_FX);
    }
}

void Game::worldSprites(bool attract) {
    auto drawFort = [&](float z, int side, bool alive, bool lit, int flash) {
        float d = z - boatZ_;
        Proj p = project(d, float(side) * 1.18f);
        if (!p.ok) return;
        float h = 46.f * p.s;
        int fog = fogFor(d);
        if (!alive) {
            spr(art_.rubble, p.x, p.y, h * 0.55f, PAL_FORT, side > 0, fog);
            return;
        }
        spr(art_.fort, p.x, p.y, h, lit ? PAL_LIT : PAL_FORT, side > 0, fog);
        spr(art_.fort, p.x, p.y + 3.f, h, PAL_FORT, side > 0, fog, true);
        if (lit) spr(art_.reticle, p.x, p.y, h * 1.25f, PAL_AMBER, false, fog);
        if (flash > 0) {
            Proj m = project(d, float(side) * 1.02f);
            if (m.ok) spr(art_.flash, m.x, m.y, 18.f * m.s, PAL_FX);
        }
    };

    if (!(inv_ > 0 && (sys_->frame / 2) % 2 == 0)) {
        Proj b = project(0.f, boatX_);
        if (b.ok) {
            float h = 54.f;
            spr(art_.boat, b.x, b.y, h, PAL_BOAT);
            spr(art_.boat, b.x + 2.f, b.y + 4.f, h, PAL_BOAT, false, 0, true);
        }
    }
    for (int i = 1; i <= 3; i++) {
        Proj w = project(-2.f - float(i) * 3.2f, boatX_);
        if (w.ok) spr(art_.wake, w.x, w.y, 8.f + float(i) * 4.f, PAL_FX, false, i * 2);
    }

    if (attract) {
        drawFort(boatZ_ + 18.f, -1, true, true, 0);
        drawFort(boatZ_ + 26.f, 1, true, false, 0);
    }
    int ord[kN];
    for (int i = 0; i < kN; i++) ord[i] = i;
    std::sort(ord, ord + kN, [&](int a, int c) { return fort_[a].z - boatZ_ < fort_[c].z - boatZ_; });
    for (int n = 0; n < kN; n++) {
        const Fort& f = fort_[ord[n]];
        if (!f.alive) continue;
        float d = f.z - boatZ_;
        bool lit = !f.doomed && d >= kGunNear && d <= kGunFar && mode_ == Mode::Run;
        drawFort(f.z, f.side, true, lit, f.flash);
    }
    for (const auto& g : graves_) drawFort(g.z, g.side, false, false, 0);

    for (float z = 40.f; z < kMouth; z += 46.f) {
        bool nearFort = false;
        for (const auto& f : fort_)
            if (std::fabs(f.z - z) < 16.f) nearFort = true;
        if (nearFort) continue;
        for (int s = -1; s <= 1; s += 2) {
            Proj p = project(z - boatZ_, float(s) * 1.42f);
            if (!p.ok) continue;
            spr(art_.lamp, p.x, p.y, 26.f * p.s, PAL_QUAY, s > 0, fogFor(z - boatZ_));
        }
    }
    for (float z = 28.f; z < kMouth; z += 54.f) {
        for (int s = -1; s <= 1; s += 2) {
            Proj p = project(z - boatZ_, float(s) * 0.62f);
            if (!p.ok) continue;
            spr(art_.buoy, p.x, p.y, 16.f * p.s, PAL_QUAY, false, fogFor(z - boatZ_));
        }
    }
    for (int s = -1; s <= 1; s += 2) {
        Proj p = project(kMouth - boatZ_, float(s) * 1.32f);
        if (!p.ok) continue;
        spr(art_.tower, p.x, p.y, 78.f * p.s, PAL_SKY, s > 0, fogFor(kMouth - boatZ_));
    }

    float drift = std::fmod(20.f + float(sys_->frame) * 0.18f, 400.f) - 40.f;
    spr(art_.cloud, drift, 18, 26, PAL_SKY);
    spr(art_.cloud, std::fmod(180.f + float(sys_->frame) * 0.11f, 420.f) - 30.f, 34, 18, PAL_SKY);
    spr(art_.sun, 262, 22, 30, PAL_SKY);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    if (mode_ == Mode::Title) {
        textC("S3 HARBOR", 4, PAL_AMBER, 2);
        textC("ONE CHANNEL", 40, PAL_TEXT, 1);
        textC("THE FORTS SHOOT BACK", 56, PAL_DIM, 1);
        textC("MAGAZINE IS FINITE", 100, PAL_AMBER, 1);
        textC("Z PORT   C STARBOARD", 116, PAL_TEXT, 1);
        textC("ARROWS STEER   START", 132, PAL_TEXT, 1);
        plate(12, 94, 296, 56);
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (mode_ == Mode::Won) {
            textC("CHANNEL CLEAR", 88, PAL_AMBER, 1);
            textC("THE FORTS ARE SILENT", 110, PAL_TEXT, 1);
        } else {
            textC("CHANNEL LOST", 88, PAL_RED, 1);
            textC(cause_, 110, PAL_TEXT, 1);
        }
        char buf[40];
        std::snprintf(buf, sizeof buf, "MAG %d  HULL %d", mag_, std::max(0, hull_));
        textC(buf, 128, PAL_DIM, 1);
        plate(28, 82, 264, 64);
    }
    if (mode_ == Mode::Run) hud();
    if (mode_ != Mode::Title) combatSprites();
    skyRoad();
    worldSprites(mode_ == Mode::Title);
}

}  // namespace harbor

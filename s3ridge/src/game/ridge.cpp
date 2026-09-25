#include "game/ridge.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace ridge {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kHorizon = 52.f;
constexpr float kZScale = 300.f;
constexpr float kZLine = 2.5f;
constexpr float kZSpawn = 18.f;
constexpr float kMove = 280.f;
constexpr float kHold = 0.36f;
constexpr float kPlant = 0.52f;
constexpr float kBoltV = 16.f;
constexpr float kCool = 0.24f;
constexpr float kFeet = 200.f;
constexpr int kStakes = 4;
constexpr int kCrests = 3;

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int p, int q) { return int(std::lround(p + (q - p) * t)); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

int palOf(Kind k) { return k == Kind::Brute ? PAL_BRUTE : k == Kind::Runner ? PAL_RUN : PAL_RAID; }
int hpOf(Kind k) { return k == Kind::Brute ? 2 : 1; }
int ptsOf(Kind k) { return k == Kind::Brute ? 180 : k == Kind::Runner ? 120 : 100; }
float radOf(Kind k) { return k == Kind::Brute ? 0.22f : 0.16f; }
float tallOf(Kind k) { return k == Kind::Brute ? 96.f : k == Kind::Runner ? 78.f : 86.f; }

}  // namespace

float Game::rowAt(float z) const { return kZScale / std::max(z, 0.75f); }

float Game::bend(float row) const { return std::sin(row * 0.02f + 0.35f) * (5.f + row * 0.04f); }

float Game::halfW(float row) const { return 32.f + row * 0.48f; }

float Game::laneX(float u) const {
    float row = rowAt(kZLine);
    return 160.f + bend(row) + u * halfW(row);
}

float Game::playerU() const {
    float row = rowAt(kZLine);
    float hw = halfW(row);
    float cx = 160.f + bend(row);
    return std::clamp((px_ - cx) / hw, -0.94f, 0.94f);
}

float Game::holdNow() const { return plant_ ? kPlant : kHold; }

float Game::speed() const {
    if (crest_ <= 0) return 3.45f;
    if (crest_ == 1) return 4.05f;
    return 4.9f;
}

Game::Spot Game::spot(float u, float z, float base) const {
    float row = rowAt(z);
    float t = std::clamp(row / rowAt(kZLine), 0.02f, 1.25f);
    float h = std::max(13.f, base * std::pow(t, 0.62f));
    float along = std::clamp((z - kZLine) / (kZSpawn - kZLine), 0.f, 1.f);
    float lift = std::pow(along, 1.3f) * h * 0.72f;
    Spot s;
    s.h = h;
    s.x = 160.f + bend(row) + u * halfW(row) + shx_;
    s.y = kHorizon + row - lift + shy_;
    s.fog = int(std::clamp(along * 3.f, 0.f, 4.f));
    return s;
}

const gs::Mipped& Game::foeImg(Kind k, int frame) const {
    if (k == Kind::Brute) return art_.brute[frame & 1];
    if (k == Kind::Runner) return art_.runner[frame & 1];
    return art_.raider[frame & 1];
}

const char* Game::crestName() const {
    static const char* names[] = {"FIRST LIGHT", "THE COLUMN", "LAST OF THEM"};
    int i = std::clamp(crest_, 0, 2);
    return names[i];
}

void Game::applySky() {
    struct Sky {
        uint16_t top, hor, drop;
        int r, g, b;
    };
    static const Sky k[3] = {
        {gs::rgb4(2, 4, 9), gs::rgb4(13, 10, 7), gs::rgb4(1, 1, 2), 48, 32, 18},
        {gs::rgb4(3, 5, 8), gs::rgb4(10, 11, 12), gs::rgb4(1, 2, 3), 28, 36, 42},
        {gs::rgb4(4, 2, 6), gs::rgb4(13, 6, 4), gs::rgb4(2, 1, 2), 64, 22, 16},
    };
    const Sky& s = k[std::clamp(crest_, 0, 2)];
    skyTop_ = s.top;
    skyHor_ = s.hor;
    skyDrop_ = s.drop;
    if (!sys_) return;
    sys_->vdp.setFogColor(skyHor_);
    sys_->setLight(s.r, s.g, s.b);
}

void Game::fillSpawns() {
    spawns_.clear();
    cursor_ = 0;
    spawned_ = 0;
    foes_.clear();
    bolts_.clear();
    struct Spec {
        float u;
        int kind;
    };
    static const Spec a[] = {{-0.40f, 0}, {0.46f, 0}, {-0.08f, 0}, {0.58f, 0}, {-0.52f, 0}, {0.18f, 0}};
    static const Spec b[] = {{-0.55f, 0}, {0.32f, 1}, {0.66f, 0}, {-0.22f, 0}, {-0.68f, 1}, {0.48f, 0}, {0.12f, 1},
                              {-0.44f, 0}};
    static const Spec c[] = {{-0.64f, 2}, {0.60f, 0}, {-0.18f, 2}, {0.70f, 2}, {-0.55f, 0},
                              {0.22f, 2},  {-0.70f, 0}, {0.48f, 2}, {0.02f, 2}, {-0.36f, 0}};
    const Spec* tab = crest_ <= 0 ? a : crest_ == 1 ? b : c;
    int n = crest_ <= 0 ? 6 : crest_ == 1 ? 8 : 10;
    float gap = crest_ <= 0 ? 1.60f : crest_ == 1 ? 1.35f : 1.12f;
    for (int i = 0; i < n; i++) {
        Spawn s;
        s.t = 0.50f + gap * float(i);
        s.u = tab[i].u;
        s.kind = Kind(tab[i].kind);
        spawns_.push_back(s);
    }
    applySky();
}

void Game::beginWatch() {
    crest_ = 0;
    score_ = 0;
    stakes_ = kStakes;
    stopped_ = 0;
    won_ = false;
    over_ = false;
    plant_ = false;
    fireCd_ = 0;
    flashT_ = 0;
    shake_ = 0;
    waveT_ = 0;
    modeT_ = 0;
    px_ = laneX(0);
    vx_ = 0;
    puffs_.clear();
    pops_.clear();
    fillSpawns();
    mode_ = Mode::Brief;
}

void Game::fanfare(bool big) {
    fanBig_ = big;
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::puffAt(float u, float z) {
    puffs_.push_back({u, z, 0.42f});
    if (puffs_.size() > 10) puffs_.erase(puffs_.begin());
}

void Game::popAt(float x, float y, int pts) {
    pops_.push_back({x, y, 0.7f, pts});
    if (pops_.size() > 4) pops_.erase(pops_.begin());
}

void Game::stop(Foe& f, bool atLine) {
    if (!f.on) return;
    f.on = false;
    score_ += f.points;
    stopped_++;
    Spot s = spot(f.u, std::max(f.z, kZLine), tallOf(f.kind));
    puffAt(f.u, f.z);
    popAt(s.x, s.y - s.h * 0.6f, f.points);
    if (atLine) {
        sys_->apu.tone(0, 196.f, 0.07f);
        sys_->apu.noiseBurst(0.18f, 420.f, 0.08f);
        beep_ = 0.08f;
    } else {
        sys_->apu.tone(0, 520.f, 0.05f);
        beep_ = 0.05f;
    }
}

void Game::breach() {
    if (mode_ != Mode::Fly) return;
    stakes_--;
    shake_ = 1.f;
    sys_->rumble(0.75f, 0.4f, 160);
    sys_->apu.noiseBurst(0.48f, 220.f, 0.28f);
    sys_->apu.tone(0, 90.f, 0.1f);
    beep_ = 0.12f;
    sys_->setLight(90, 18, 14);
    if (stakes_ <= 0) {
        stakes_ = 0;
        mode_ = Mode::Lost;
        won_ = false;
        over_ = true;
        modeT_ = 0;
    }
}

void Game::loose() {
    if (fireCd_ > 0 || bolts_.size() >= 8) return;
    fireCd_ = kCool;
    flashT_ = 0.08f;
    Bolt b;
    b.u = playerU();
    b.z = kZLine + 0.25f;
    b.on = true;
    bolts_.push_back(b);
    sys_->apu.tone(0, 740.f, 0.045f);
    beep_ = 0.04f;
}

void Game::tickAudio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (cue_ > 0) {
        cue_ -= DT;
        if (cue_ <= 0) sys_->apu.tone(2, 0, 0);
    }
    if (fanStep_ < 0) return;
    fanT_ += DT;
    static const float big[] = {349.2f, 440.0f, 523.3f, 698.5f};
    static const float small[] = {392.0f, 523.3f, 659.3f};
    const float* notes = fanBig_ ? big : small;
    int n = fanBig_ ? 4 : 3;
    if (fanT_ < 0.13f) return;
    if (fanStep_ < n) sys_->apu.tone(1, notes[fanStep_], 0.07f);
    else sys_->apu.tone(1, 0, 0);
    fanStep_++;
    fanT_ = 0;
    if (fanStep_ > n + 2) fanStep_ = -1;
}

void Game::update(float dt) {
    const Foe* best = nullptr;
    for (const Foe& f : foes_) {
        if (!f.on) continue;
        if (!best || f.z < best->z) best = &f;
    }
    float dir = 0;
    bool want = false;
    plant_ = false;
    if (bot_) {
        if (best) {
            float tx = laneX(best->u);
            float dx = tx - px_;
            if (dx > 6.f) dir = 1.f;
            else if (dx < -6.f) dir = -1.f;
            if (std::fabs(dx) < 10.f && best->z < 15.f && best->z > kZLine) want = true;
        } else {
            float dx = laneX(0) - px_;
            if (dx > 8.f) dir = 1.f;
            else if (dx < -8.f) dir = -1.f;
        }
    } else {
        const gs::Pad& pad = sys_->pad;
        plant_ = pad.down(gs::BTN_DOWN);
        if (!plant_) {
            if (std::fabs(pad.axisX) > 0.12f) dir = std::clamp(pad.axisX, -1.f, 1.f);
            else dir = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        }
        want = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
    }

    float row = rowAt(kZLine);
    float minX = 160.f + bend(row) - halfW(row) * 0.92f;
    float maxX = 160.f + bend(row) + halfW(row) * 0.92f;
    float nx = std::clamp(px_ + dir * kMove * dt, minX, maxX);
    vx_ = (nx - px_) / dt;
    px_ = nx;
    if (want) loose();

    while (cursor_ < int(spawns_.size()) && spawns_[size_t(cursor_)].t <= waveT_) {
        const Spawn& sp = spawns_[size_t(cursor_++)];
        Foe f;
        f.kind = sp.kind;
        f.u = sp.u;
        f.z = kZSpawn;
        f.age = 0;
        f.flash = 0;
        f.hp = hpOf(sp.kind);
        f.points = ptsOf(sp.kind);
        f.on = true;
        foes_.push_back(f);
        spawned_++;
        sys_->apu.tone(2, 128.f, 0.04f);
        cue_ = 0.06f;
    }

    for (Bolt& b : bolts_) {
        if (!b.on) continue;
        b.z += kBoltV * dt;
        if (b.z > kZSpawn + 1.5f) {
            b.on = false;
            continue;
        }
        Foe* hit = nullptr;
        for (Foe& f : foes_) {
            if (!f.on || f.hp <= 0) continue;
            if (std::fabs(b.u - f.u) > radOf(f.kind)) continue;
            if (std::fabs(b.z - f.z) > 0.9f) continue;
            if (!hit || f.z < hit->z) hit = &f;
        }
        if (!hit) continue;
        b.on = false;
        hit->hp--;
        hit->flash = 0.12f;
        puffAt(hit->u, hit->z);
        sys_->apu.tone(0, hit->hp > 0 ? 300.f : 520.f, 0.04f);
        beep_ = 0.04f;
        if (hit->hp <= 0) stop(*hit, false);
    }

    for (Foe& f : foes_) {
        if (!f.on || mode_ != Mode::Fly) continue;
        f.z -= speed() * dt;
        f.age += dt;
        if (f.flash > 0) f.flash -= dt;
        if (f.z > kZLine) continue;
        if (std::fabs(f.u - playerU()) <= holdNow()) stop(f, true);
        else {
            f.on = false;
            puffAt(f.u, kZLine);
            breach();
        }
    }

    foes_.erase(std::remove_if(foes_.begin(), foes_.end(), [](const Foe& f) { return !f.on; }), foes_.end());
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(), [](const Bolt& b) { return !b.on; }), bolts_.end());

    if (mode_ != Mode::Fly || stakes_ <= 0) return;
    if (cursor_ >= int(spawns_.size()) && foes_.empty() && spawned_ > 0) {
        if (crest_ + 1 >= kCrests) {
            mode_ = Mode::Victory;
            won_ = true;
            over_ = true;
            modeT_ = 0;
            fanfare(true);
            sys_->rumble(0.25f, 0.45f, 220);
            sys_->setLight(40, 80, 36);
        } else {
            mode_ = Mode::Clear;
            modeT_ = 0;
            fanfare(false);
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

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 2.f || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.y > gs::SCREEN_H + 48 || s.x + s.w < -48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 6.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 6L, 420L));
    s.h = int16_t(std::max(6L, std::lround(double(w) * 0.16)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = 0;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(modeT_ * 73.f) * 5.f * shake_;
        shy_ = std::cos(modeT_ * 59.f) * 3.f * shake_;
    }

    int hor = std::clamp(int(std::lround(kHorizon + shy_)), 36, 80);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor) {
            float t = float(y) / float(std::max(hor, 1));
            v.lineBackdrop[y] = mix(skyTop_, skyHor_, t * t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor);
        float z = kZScale / row;
        r.on = true;
        r.cx = 160.f + bend(row) + shx_;
        r.hw = halfW(row);
        r.v = z * 110.f - scroll_ * 55.f;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(r.v / 36.f)) & 1) ? 1 : 0;
        r.style = gs::ROAD_ROCKY;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(11.f - row * 0.12f), 0, 10));
        float dropT = std::clamp(row / 90.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(skyDrop_, gs::rgb4(0, 0, 1), dropT);
    }
    v.setFogColor(skyHor_);

    for (Pop& p : pops_) {
        if (p.t > 0) text("+" + std::to_string(p.pts), p.x, p.y, 0.62f, PAL_AMBER);
        p.t -= DT;
        p.y -= 16.f * DT;
    }
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0; }), pops_.end());

    char buf[48];
    if (mode_ == Mode::Title) text("S3 RIDGE", 160, 40, 1.25f, PAL_AMBER);
    else if (mode_ == Mode::Brief) text(crestName(), 160, 64, 1.05f, PAL_AMBER);
    else if (mode_ == Mode::Clear) text("CREST HELD", 160, 68, 1.05f, PAL_GREEN);
    else if (mode_ == Mode::Victory) text("THE PATH HELD", 160, 62, 0.95f, PAL_GREEN);
    else if (mode_ == Mode::Lost) text("THE PATH IS LOST", 160, 62, 0.85f, PAL_RED);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 70, 1.15f, PAL_INK);

    bool moving = std::fabs(vx_) > 24.f;
    int fr = (moving && (int(modeT_ * 9.f) & 1)) ? 1 : 0;
    bool flip = vx_ < -24.f;
    float body = plant_ ? 80.f : 88.f;
    spr(art_.you[fr], px_ + shx_, kFeet + shy_, body, PAL_YOU, flip, 0, true);
    if (flashT_ > 0)
        spr(art_.flash, px_ + shx_ + (flip ? -12.f : 12.f), kFeet + shy_ - 64.f, 18, PAL_FX, flip, 0, false);
    float zone = 2.f * holdNow() * halfW(rowAt(kZLine));
    shadow(px_ + shx_, kFeet + shy_ - 6.f, zone);

    for (const Bolt& b : bolts_) {
        Spot s = spot(b.u, b.z, 26.f);
        spr(art_.bolt, s.x, s.y, std::max(10.f, s.h), PAL_FX, false, s.fog, true);
    }

    for (Puff& p : puffs_) {
        float k = std::clamp(p.t / 0.42f, 0.f, 1.f);
        Spot s = spot(p.u, p.z, 30.f);
        spr(art_.dust, s.x, s.y - s.h * 0.3f, 16.f + (1.f - k) * 28.f, PAL_FX, false, int((1.f - k) * 8.f), false);
        p.t -= DT;
    }
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());

    std::vector<int> order;
    order.reserve(foes_.size());
    for (int i = 0; i < int(foes_.size()); i++)
        if (foes_[size_t(i)].on) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return foes_[size_t(a)].z < foes_[size_t(b)].z; });
    for (int idx : order) {
        const Foe& f = foes_[size_t(idx)];
        Spot s = spot(f.u, f.z, tallOf(f.kind));
        s.x += std::sin(f.age * 10.f) * 1.2f;
        int frame = int(f.age * (f.kind == Kind::Runner ? 10.f : 7.f)) & 1;
        if (s.fog < 6) shadow(s.x, s.y, s.h * 0.42f);
        spr(foeImg(f.kind, frame), s.x, s.y, s.h, palOf(f.kind), f.u >= 0, s.fog, true);
        if (f.flash > 0) spr(art_.flash, s.x, s.y - s.h * 0.45f, s.h * 0.35f, PAL_FX, false, 0, false);
    }

    auto prop = [&](float u, float z, float base, const gs::Mipped& img, int pal) {
        Spot s = spot(u, z, base);
        spr(img, s.x, s.y, s.h, pal, u < 0, std::max(0, s.fog - 1), true);
    };
    float lineY = kHorizon + rowAt(kZLine) + shy_;
    float lineW = halfW(rowAt(kZLine)) * 1.75f;
    spr(art_.mouth, laneX(0) + shx_, lineY, lineW * (8.f / 128.f), PAL_STONE, false, 0, false);
    prop(-0.90f, 3.15f, 78.f, art_.post, PAL_STONE);
    prop(0.90f, 3.15f, 78.f, art_.post, PAL_STONE);
    prop(-0.78f, 6.4f, 52.f, art_.cairn, PAL_STONE);
    prop(0.80f, 9.1f, 48.f, art_.cairn, PAL_STONE);
    prop(-0.70f, 12.2f, 44.f, art_.cairn, PAL_STONE);
    prop(0.04f, 15.2f, 100.f, art_.banner, PAL_BANNER);

    if (mode_ == Mode::Title || mode_ == Mode::Brief) {
        const float us[3] = {-0.28f, 0.18f, 0.46f};
        const Kind ks[3] = {Kind::Raider, Kind::Runner, Kind::Brute};
        for (int i = 0; i < 3; i++) {
            float ph = std::fmod(scroll_ * 0.18f + i * 0.33f, 1.f);
            float z = kZSpawn - ph * 5.5f;
            Spot s = spot(us[i], z, tallOf(ks[i]));
            int frame = int(scroll_ * 8.f + float(i)) & 1;
            spr(foeImg(ks[i], frame), s.x, s.y, s.h, palOf(ks[i]), us[i] > 0, s.fog, true);
        }
    }

    spr(art_.mount[0], 54 + shx_ * 0.3f, kHorizon + shy_ + 4, 62, PAL_MOUNT, false, 1, true);
    spr(art_.mount[1], 268 + shx_ * 0.3f, kHorizon + shy_ + 8, 54, PAL_MOUNT, false, 2, true);
    float drift = std::fmod(scroll_ * 16.f, 420.f);
    spr(art_.cloud, drift - 50.f, 22.f + shy_, 18, PAL_INK, false, 2, false);
    spr(art_.cloud, std::fmod(drift + 200.f, 420.f) - 50.f, 34.f, 13, PAL_INK, true, 3, false);
    float sunX = crest_ == 2 ? 118.f : 196.f;
    spr(art_.sun, sunX, crest_ == 1 ? 26.f : 22.f, crest_ == 2 ? 18.f : 26.f, PAL_FX, false, 0, false);

    if (mode_ == Mode::Fly || mode_ == Mode::Pause || mode_ == Mode::Clear) {
        std::snprintf(buf, sizeof buf, "CREST %d", crest_ + 1);
        hud(1, 1, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(39 - int(std::strlen(buf)), 1, buf, PAL_AMBER);
        std::string stakes = "STAKES ";
        for (int i = 0; i < kStakes; i++) stakes += i < stakes_ ? '|' : '.';
        hud(1, 26, stakes, stakes_ <= 2 ? PAL_RED : PAL_INK);
        hud(31, 26, "BOLT", fireCd_ <= 0 ? PAL_GREEN : PAL_INK);
        if (plant_) hud(24, 26, "PIKE", PAL_AMBER);
        const Foe* near = nullptr;
        for (const Foe& f : foes_) {
            if (!f.on || f.z > 6.2f) continue;
            if (!near || f.z < near->z) near = &f;
        }
        if (near && mode_ == Mode::Fly && std::fabs(near->u - playerU()) > holdNow()) hudC(3, "STEP THE PATH", PAL_RED);
    }

    if (mode_ == Mode::Title) {
        hudC(12, "THEY COME OVER THE CREST", PAL_AMBER);
        hudC(14, "HOLD THE PATH", PAL_INK);
        hudC(17, "ARROWS  STEP THE PATH", PAL_INK);
        hudC(19, "DOWN    PLANT YOUR PIKE", PAL_INK);
        hudC(21, "C OR Z  LOOSE A BOLT", PAL_INK);
        if ((int(modeT_ * 2.f) & 1) == 0) hudC(24, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 26, S3_VERSION_STRING, PAL_INK);
    } else if (mode_ == Mode::Brief) {
        hudC(13, "THEY COME OVER THE CREST", PAL_INK);
        hudC(15, "HOLD THE PATH", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "CREST %d OF %d", crest_ + 1, kCrests);
        hudC(17, buf, PAL_INK);
        if ((int(modeT_ * 2.f) & 1) == 0) hudC(21, "PRESS START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        hudC(16, "START  RESUME", PAL_INK);
        hudC(18, "ESC    TITLE", PAL_INK);
    } else if (mode_ == Mode::Clear) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(14, buf, PAL_INK);
        if (crest_ + 1 < kCrests) hudC(17, "THE NEXT CREST IS COMING", PAL_AMBER);
        if ((int(modeT_ * 2.f) & 1) == 0) hudC(21, "PRESS START", PAL_INK);
    } else if (mode_ == Mode::Victory) {
        hudC(12, "THEY DID NOT TAKE IT", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "STOPPED %d", stopped_);
        hudC(15, buf, PAL_GREEN);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(17, buf, PAL_INK);
        std::snprintf(buf, sizeof buf, "STAKES %d", stakes_);
        hudC(19, buf, PAL_INK);
        hudC(23, "START", PAL_AMBER);
    } else if (mode_ == Mode::Lost) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(14, buf, PAL_INK);
        hudC(17, crestName(), PAL_AMBER);
        hudC(21, "START", PAL_INK);
    }
}

int Game::marker() const {
    switch (mode_) {
    case Mode::Title: return 0;
    case Mode::Fly: return 1;
    case Mode::Clear:
    case Mode::Brief:
    case Mode::Pause: return 2;
    case Mode::Victory: return 3;
    case Mode::Lost: return 4;
    }
    return 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    crest_ = 0;
    applySky();
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.2f, 0.28f, 0.16f);
    px_ = laneX(0);
    if (bot_) beginWatch();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    modeT_ += DT;
    if (mode_ != Mode::Pause) scroll_ += DT;
    if (fireCd_ > 0) fireCd_ -= DT;
    if (flashT_ > 0) flashT_ -= DT;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    tickAudio();

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        float u = std::sin(modeT_ * 0.8f) * 0.42f;
        float x = laneX(u);
        vx_ = (x - px_) / DT;
        px_ = x;
        plant_ = false;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            beginWatch();
            sys.apu.tone(0, 660.f, 0.05f);
            beep_ = 0.05f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Brief) {
        vx_ = 0;
        if ((bot_ && modeT_ > 0.32f) || pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Fly;
            waveT_ = 0;
            modeT_ = 0;
            sys.apu.tone(0, 440.f, 0.04f);
            beep_ = 0.04f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            modeT_ = 0;
            crest_ = 0;
            applySky();
        }
    } else if (mode_ == Mode::Fly) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            modeT_ = 0;
        } else {
            waveT_ += DT;
            update(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Fly;
            modeT_ = 0;
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            modeT_ = 0;
            plant_ = false;
            crest_ = 0;
            applySky();
        }
    } else if (mode_ == Mode::Clear) {
        vx_ = 0;
        if ((bot_ && modeT_ > 0.38f) || pad.pressed(gs::BTN_START) || modeT_ > 3.2f) {
            crest_++;
            waveT_ = 0;
            modeT_ = 0;
            px_ = laneX(0);
            fillSpawns();
            mode_ = Mode::Brief;
        }
    } else if (mode_ == Mode::Victory || mode_ == Mode::Lost) {
        vx_ *= 0.9f;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = Mode::Title;
            modeT_ = 0;
            over_ = false;
            won_ = false;
            plant_ = false;
            crest_ = 0;
            foes_.clear();
            bolts_.clear();
            applySky();
        }
    }

    draw();
}

}  // namespace ridge

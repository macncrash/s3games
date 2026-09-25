#include "game/blitz.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace blitz {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpeed = 26.f;
constexpr float kHalf = 3.08f;
constexpr float kWall = 3.62f;
constexpr float kFocal = 210.f;
constexpr float kCamH = 2.35f;
constexpr float kHor = 74.f;
constexpr float kShipDz = 2.7f;
constexpr float kShipH = 0.42f;
constexpr float kLat = 8.2f;
constexpr float kAcc = 36.f;
constexpr float kAltV = 1.05f;
constexpr float kAmmo0 = 48.f;
constexpr float kDrain = 0.90f;
constexpr float kShotCost = 1.f;
constexpr float kRefund = 6.f;
constexpr float kCellGain = 5.f;
constexpr float kCool = 0.18f;
constexpr float kBoltRel = 100.f;
constexpr float kPortZ = 1360.f;
constexpr float kLowCut = 1.72f;
constexpr float kHighCut = 0.98f;
constexpr int kHp = 2;

struct Proto {
    float z, x, w;
    int kind;
    int gate;
};

// Local x is across the trench. The magazine drains by itself; hits put rounds back.
const Proto kCourse[] = {
    {120, 1.60f, 0.55f, 0, 0},
    {200, -1.80f, 0.48f, 4, 0},
    {260, 0.00f, 3.70f, 1, 0},
    {360, -1.40f, 0.55f, 0, 0},
    {450, 2.00f, 1.15f, 3, 0},
    {520, 0.20f, 0.55f, 0, 0},
    {620, 0.00f, 3.70f, 2, 0},
    {720, -2.00f, 0.55f, 0, 0},
    {800, 1.50f, 0.46f, 4, 0},
    {900, 1.70f, 0.55f, 0, 0},
    {980, -1.90f, 1.20f, 3, 0},
    {1080, -2.55f, 1.35f, 3, 0},
    {1080, 2.55f, 1.35f, 3, 0},
    {1080, 0.00f, 0.74f, 0, 1},
    {1180, 0.00f, 0.50f, 4, 0},
    {1260, -1.20f, 0.55f, 0, 0},
    {kPortZ, 0.00f, 2.60f, 5, 0},
};

struct Cue {
    float z, x, alt;
    bool fire;
};

const Cue kCues[] = {
    {0, 0.f, 0.50f, false},
    {70, 1.60f, 0.50f, true},
    {102, 0.15f, 0.50f, false},
    {155, -1.55f, 0.50f, false},
    {215, 0.f, 1.f, false},
    {300, -1.40f, 0.50f, true},
    {342, -0.15f, 0.50f, false},
    {400, -1.15f, 0.50f, false},
    {470, 0.20f, 0.50f, true},
    {502, 0.f, 0.50f, false},
    {575, 0.f, 0.f, false},
    {660, -2.00f, 0.50f, true},
    {702, 0.15f, 0.50f, false},
    {755, 1.40f, 0.50f, false},
    {845, 1.70f, 0.50f, true},
    {882, 1.15f, 0.50f, false},
    {935, 1.35f, 0.50f, false},
    {990, 0.f, 0.50f, true},
    {1062, 0.f, 0.50f, false},
    {1140, 0.f, 0.50f, false},
    {1210, -1.20f, 0.50f, true},
    {1242, 0.f, 0.50f, false},
    {1310, 0.f, 0.50f, false},
};

float centerAt(float z) {
    return 9.f * std::sin(z * 0.010f) + 4.5f * std::sin(z * 0.023f + 0.7f);
}

int clampi(int v, int lo, int hi) { return std::max(lo, std::min(hi, v)); }

}  // namespace

float Game::noseY() const { return 0.70f + alt_ * 1.25f; }

float Game::worldX(float z, float local) const { return centerAt(z) + local; }

int Game::rounds() const {
    if (ammo_ <= 0.f) return 0;
    return int(std::ceil(ammo_ - 1e-4f));
}

int Game::score() const { return rounds() * 10 + towers_ * 100 + cells_ * 40; }

void Game::loadMarks() {
    marks_.clear();
    marks_.reserve(sizeof kCourse / sizeof kCourse[0]);
    for (const Proto& p : kCourse) {
        Mark m;
        m.z = p.z;
        m.x = p.x;
        m.w = p.w;
        m.kind = Kind(p.kind);
        m.hp = m.kind == Kind::Turret ? kHp : 1;
        m.live = true;
        m.gate = p.gate != 0;
        marks_.push_back(m);
    }
}

void Game::newRun() {
    dist_ = 0;
    localX_ = 0;
    vx_ = 0;
    alt_ = 0.50f;
    ammo_ = kAmmo0;
    cool_ = 0;
    shake_ = 0;
    flash_ = 0;
    shotSnd_ = 0;
    tickSnd_ = 0;
    towers_ = 0;
    cells_ = 0;
    shots_ = 0;
    lastShown_ = -1;
    won_ = false;
    over_ = false;
    note_[0] = 0;
    t_ = 0;
    bolts_.clear();
    booms_.clear();
    loadMarks();
    mode_ = Mode::Run;
}

void Game::fail(const char* why) {
    if (over_) return;
    over_ = true;
    won_ = false;
    mode_ = Mode::Dead;
    flash_ = 1.f;
    shake_ = 1.f;
    std::snprintf(note_, sizeof note_, "%s", why);
    if (sys_ && !sys_->headless) sys_->rumble(0.7f, 0.9f, 220);
    if (sys_) sys_->apu.noiseBurst(0.22f, 700.f, 0.35f);
}

void Game::win() {
    if (over_) return;
    over_ = true;
    won_ = true;
    mode_ = Mode::Win;
    flash_ = 0.35f;
    std::snprintf(note_, sizeof note_, "PORT");
    if (sys_ && !sys_->headless) {
        sys_->rumble(0.3f, 0.5f, 160);
        sys_->setLight(40, 180, 70);
    }
}

void Game::boomAt(float local, float y, float z) {
    booms_.push_back({local, y, z, 0});
    shake_ = std::min(1.f, shake_ + 0.45f);
    flash_ = std::min(1.f, flash_ + 0.28f);
}

void Game::killTurret(Mark& m) {
    if (!m.live) return;
    m.live = false;
    ammo_ = std::min(96.f, ammo_ + kRefund);
    ++towers_;
    boomAt(m.x, 0.85f, m.z);
    if (sys_) sys_->apu.noiseBurst(0.12f, 1400.f, 0.12f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    loadMarks();
    mode_ = Mode::Title;
    t_ = 0;
    dist_ = 40;
    localX_ = 0;
    vx_ = 0;
    alt_ = 0.5f;
    ammo_ = kAmmo0;
    over_ = false;
    won_ = false;
    note_[0] = 0;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    sys.vdp.hudEnabled = true;
}

Game::Proj Game::project(float wx, float wy, float wz) const {
    Proj p;
    const float dz = wz - dist_;
    if (dz < 0.75f || dz > 130.f) return p;
    p.s = kFocal / dz;
    const float sh = shake_ > 0 ? std::sin(t_ * 80.f) * shake_ * 4.f : 0.f;
    p.x = 160.f + (wx - worldX(dist_, localX_)) * p.s + sh;
    p.y = kHor + (kCamH - wy) * p.s;
    p.fog = int(std::min(16.f, std::max(0.f, (dz - 16.f) * 0.32f)));
    p.ok = true;
    return p;
}

void Game::quad(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool flip, bool shadow) {
    if (!(w > 0.8f) || !(h > 0.8f) || m.h <= 0 || !sys_) return;
    if (cx + w * 0.5f < -30 || cx - w * 0.5f > 350 || cy + h * 0.5f < -30 || cy - h * 0.5f > 250) return;
    if (w > 460.f) {
        cx += (w - 460.f) * 0.5f;
        w = 460.f;
    }
    if (h > 300.f) {
        cy += (h - 300.f) * 0.5f;
        h = 300.f;
    }
    gs::Sprite s;
    s.w = std::max(1, int(std::lround(w)));
    s.h = std::max(1, int(std::lround(h)));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(clampi(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || !sys_ || row < 0 || row > 27) return;
    for (int i = 0; s[i]; ++i) {
        const int x = col + i;
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) ++n;
    hud(20 - n / 2, row, s, pal);
}

const char* Game::hint() const {
    float gateDz = 1e9f;
    float best = 1e9f;
    const char* text = nullptr;
    for (const Mark& m : marks_) {
        if (!m.live) continue;
        const float dz = m.z - dist_;
        if (dz < 4.f || dz > 78.f) continue;
        if (m.gate && m.kind == Kind::Turret && dz < gateDz) gateDz = dz;
        if (dz >= best) continue;
        best = dz;
        switch (m.kind) {
        case Kind::SparLow: text = "CLIMB"; break;
        case Kind::SparHigh: text = "DIVE"; break;
        case Kind::Block: text = "BANK"; break;
        case Kind::Turret: text = m.gate ? "SHOOT THE PLUG" : "SHOOT"; break;
        case Kind::Cell: text = "CELL"; break;
        case Kind::Port: text = "PORT AHEAD"; break;
        }
    }
    if (gateDz < 80.f) return "SHOOT THE PLUG";
    return text;
}

Game::Stick Game::botStick() const {
    const Cue* cue = &kCues[0];
    for (const Cue& q : kCues)
        if (dist_ >= q.z) cue = &q;
    Stick s;
    s.steer = std::clamp((cue->x - localX_) * 1.2f, -1.f, 1.f);
    if (localX_ > kHalf - 0.55f) s.steer = -1.f;
    if (localX_ < -kHalf + 0.55f) s.steer = 1.f;
    if (alt_ < cue->alt - 0.03f) s.climb = 1.f;
    else if (alt_ > cue->alt + 0.03f) s.climb = -1.f;
    if (!cue->fire) return s;
    for (const Mark& m : marks_) {
        if (!m.live || m.kind != Kind::Turret) continue;
        const float dz = m.z - dist_;
        if (dz < 8.f || dz > 72.f) continue;
        if (std::fabs(m.x - localX_) > 0.46f) continue;
        int inbound = 0;
        for (const Bolt& b : bolts_) {
            if (b.z <= m.z && b.z > dist_ && std::fabs(b.local - m.x) < 0.75f) ++inbound;
        }
        if (inbound < m.hp) s.fire = true;
    }
    return s;
}

Game::Stick Game::stick() const {
    if (bot_) return botStick();
    Stick s;
    if (!sys_) return s;
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_LEFT)) s.steer -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) s.steer += 1.f;
    if (std::fabs(pad.axisX) > 0.2f) s.steer = pad.axisX;
    s.steer = std::clamp(s.steer, -1.f, 1.f);
    if (pad.down(gs::BTN_UP)) s.climb += 1.f;
    if (pad.down(gs::BTN_DOWN)) s.climb -= 1.f;
    s.climb = std::clamp(s.climb, -1.f, 1.f);
    s.fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
    return s;
}

void Game::update() {
    if (mode_ != Mode::Run) return;
    t_ += kDt;
    const Stick in = stick();
    const float goalV = in.steer * kLat;
    vx_ += std::clamp(goalV - vx_, -kAcc * kDt, kAcc * kDt);
    localX_ += vx_ * kDt;
    alt_ = std::clamp(alt_ + in.climb * kAltV * kDt, 0.f, 1.f);
    dist_ += kSpeed * kDt;
    cool_ = std::max(0.f, cool_ - kDt);

    const float stepZ = (kSpeed + kBoltRel) * kDt;
    for (Bolt& b : bolts_) b.z += stepZ;
    for (Bolt& b : bolts_) {
        for (Mark& m : marks_) {
            if (!m.live || m.kind != Kind::Turret) continue;
            const float prev = b.z - stepZ;
            if (m.z < prev - 0.6f || m.z > b.z + 0.6f) continue;
            if (std::fabs(b.local - m.x) > 0.75f) continue;
            b.z = -1000.f;
            if (--m.hp <= 0) killTurret(m);
            else boomAt(m.x, 0.9f, m.z);
            break;
        }
    }
    bolts_.erase(std::remove_if(bolts_.begin(), bolts_.end(),
                                [&](const Bolt& b) { return b.z < dist_ - 2.f || b.z > dist_ + 95.f; }),
                 bolts_.end());

    if (in.fire && cool_ <= 0.f && ammo_ >= kShotCost) {
        ammo_ -= kShotCost;
        cool_ = kCool;
        ++shots_;
        bolts_.push_back({localX_, noseY(), dist_ + 3.5f});
        shotSnd_ = 0.06f;
        flash_ = std::min(1.f, flash_ + 0.12f);
        if (sys_ && !sys_->headless && !bot_) sys_->rumble(0.15f, 0.35f, 40);
    }

    if (std::fabs(localX_) > kHalf) {
        fail("WALL");
        return;
    }

    for (Mark& m : marks_) {
        if (!m.live) continue;
        const float dz = m.z - dist_;
        const float adx = std::fabs(localX_ - m.x);
        if (m.kind == Kind::Cell) {
            if (std::fabs(dz) < 1.7f && adx < m.w + 0.32f) {
                m.live = false;
                ammo_ = std::min(96.f, ammo_ + kCellGain);
                ++cells_;
                boomAt(m.x, noseY(), m.z);
            }
            continue;
        }
        if (m.kind == Kind::Port) {
            if (dist_ >= m.z) {
                if (adx <= m.w) win();
                else fail("MOUTH");
                return;
            }
            continue;
        }
        if (dz < -1.3f || dz > 1.35f) continue;
        if (m.kind == Kind::SparLow) {
            if (adx < m.w + 0.2f && noseY() < kLowCut) {
                fail("HULL");
                return;
            }
        } else if (m.kind == Kind::SparHigh) {
            if (adx < m.w + 0.2f && noseY() > kHighCut) {
                fail("HULL");
                return;
            }
        } else if (adx < m.w + 0.42f) {
            fail("HULL");
            return;
        }
    }
    if (over_) return;

    ammo_ -= kDrain * kDt;
    if (ammo_ <= 0.f) {
        ammo_ = 0;
        fail("DRY");
    }
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Run) {
        a.tone(0, 48.f + alt_ * 18.f, 0.034f);
        a.tone(1, shotSnd_ > 0 ? 720.f : 0.f, shotSnd_ > 0 ? 0.045f : 0.f);
        a.tone(2, tickSnd_ > 0 ? 160.f : 0.f, tickSnd_ > 0 ? 0.028f : 0.f);
    } else if (mode_ == Mode::Win) {
        const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        const int step = std::min(3, int(t_ * 6.f));
        a.tone(0, notes[step], t_ < 0.7f ? 0.07f : 0.f);
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
    } else if (mode_ == Mode::Title) {
        a.tone(0, 70.f, 0.02f);
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
    } else {
        a.tone(0, 0, 0);
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    vdp.A.enabled = false;
    vdp.B.enabled = false;

    const float pulse = 0.5f + 0.5f * std::sin(t_ * 22.f);
    vdp.setColor(PAL_SHIP * 16 + 6, gs::rgb4(10 + int(5 * pulse), 15, 15));
    vdp.setColor(PAL_CELL * 16 + 2, gs::rgb4(10, 12 + int(3 * pulse), 15));
    const float portDz = kPortZ - dist_;
    const int horizonG = (portDz > 0.f && portDz < 55.f) ? int((55.f - portDz) * 0.12f) : 0;

    for (int y = 0; y < gs::SCREEN_H; ++y) {
        vdp.road[y].on = false;
        vdp.lineFog[y] = 0;
        if (y < int(kHor)) {
            const float u = y / kHor;
            int r = int(u * u * 2.f) + int(flash_ * 7.f);
            int g = 1 + horizonG / 2;
            int b = 1 + int(u * 4.f);
            if (y > int(kHor) - 3) {
                r = std::min(15, 6 + horizonG);
                g = std::min(15, 4 + horizonG);
                b = 3;
            }
            vdp.lineBackdrop[y] = gs::rgb4(std::min(15, r), std::min(15, g), std::min(15, b));
            continue;
        }
        vdp.lineBackdrop[y] = gs::rgb4(std::min(15, int(flash_ * 6.f)), 0, 1);
        const float dy = float(y) - kHor;
        const float dz = kCamH * kFocal / dy;
        const float wz = dist_ + dz;
        const float rel = centerAt(wz) - worldX(dist_, localX_);
        gs::RoadLine& r = vdp.road[y];
        r.on = true;
        r.cx = 160.f + rel * kFocal / dz;
        r.hw = kWall * kFocal / dz;
        r.v = wz * 16.f;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(wz / 7.f)) & 1) ? 1 : 0;
        r.style = 1;
        r.left = gs::GROUND_LAND;
        r.right = gs::GROUND_LAND;
        vdp.lineFog[y] = uint8_t(std::max(0, std::min(12, int((26.f - dy) * 0.35f))));
    }

    std::vector<int> order;
    order.reserve(marks_.size());
    for (int i = 0; i < int(marks_.size()); ++i) {
        if (!marks_[i].live && marks_[i].kind != Kind::Port) continue;
        if (marks_[i].z < dist_ - 2.f) continue;
        order.push_back(i);
    }
    std::sort(order.begin(), order.end(), [&](int a, int b) { return marks_[a].z < marks_[b].z; });
    for (int id : order) {
        const Mark& m = marks_[id];
        const float wx = worldX(m.z, m.x);
        const Proj p = project(wx, 0.f, m.z);
        if (!p.ok) continue;
        if (m.kind == Kind::Turret) {
            const float h = 1.15f * p.s;
            quad(art_.turret, p.x, p.y - h * 0.42f, h * (40.f / 52.f), h, PAL_TURRET, p.fog);
        } else if (m.kind == Kind::Block) {
            const Proj top = project(wx, 2.7f, m.z);
            const float h = std::fabs(p.y - top.y);
            const float cy = (p.y + top.y) * 0.5f;
            const float w = std::max(6.f, m.w * 2.f * p.s);
            quad(art_.block, p.x, cy, w, h, PAL_BLOCK, p.fog);
        } else if (m.kind == Kind::SparLow || m.kind == Kind::SparHigh) {
            const float y0 = m.kind == Kind::SparLow ? 0.f : kHighCut;
            const float y1 = m.kind == Kind::SparLow ? kLowCut : 3.3f;
            const Proj a = project(worldX(m.z, m.x - m.w), y0, m.z);
            const Proj b = project(worldX(m.z, m.x + m.w), y1, m.z);
            if (!a.ok || !b.ok) continue;
            const float w = std::fabs(b.x - a.x);
            const float h = std::fabs(a.y - b.y);
            quad(art_.spar, (a.x + b.x) * 0.5f, (a.y + b.y) * 0.5f, std::max(8.f, w), std::max(4.f, h), PAL_SPAR, a.fog);
        } else if (m.kind == Kind::Cell) {
            const float bob = 0.55f + 0.08f * std::sin(t_ * 6.f + m.z);
            const Proj c = project(wx, bob, m.z);
            if (!c.ok) continue;
            const float h = (0.55f + 0.08f * pulse) * c.s;
            quad(art_.cell, c.x, c.y, h, h, PAL_CELL, c.fog);
        } else if (m.kind == Kind::Port) {
            const Proj c = project(wx, 1.25f, m.z);
            if (!c.ok) continue;
            const float h = 3.15f * c.s;
            quad(art_.port, c.x, c.y, h, h, PAL_PORT, std::max(0, c.fog - 4));
        }
    }

    for (const Boom& b : booms_) {
        const Proj p = project(worldX(b.z, b.local), b.y, b.z);
        if (!p.ok) continue;
        const float h = (0.7f + b.t * 2.8f) * p.s;
        quad(art_.boom, p.x, p.y, h, h, PAL_BOOM, std::min(16, p.fog + int(b.t * 24.f)));
    }
    for (const Bolt& b : bolts_) {
        const Proj p = project(worldX(b.z, b.local), b.y, b.z);
        if (!p.ok) continue;
        const float h = 0.42f * p.s;
        quad(art_.bolt, p.x, p.y, h * 0.4f, h, PAL_BOLT, p.fog);
    }

    if (mode_ != Mode::Title || true) {
        const float sz = dist_ + kShipDz;
        const Proj nose = project(worldX(sz, localX_), noseY(), sz);
        const Proj deck = project(worldX(sz, localX_), 0.18f, sz);
        if (deck.ok) quad(art_.shadow, deck.x, deck.y, 1.15f * deck.s, 0.28f * deck.s, PAL_SHIP, deck.fog, false, true);
        if (nose.ok) {
            const int bank = vx_ > 2.4f ? 2 : vx_ < -2.4f ? 0 : 1;
            const float h = kShipH * nose.s;
            quad(art_.ship[bank], nose.x, nose.y, h * (80.f / 64.f), h, PAL_SHIP, 0);
        }
    }

    const float z0 = std::floor(dist_ / 8.f) * 8.f;
    for (int i = 0; i <= 9; ++i) {
        const float z = z0 + float(i) * 8.f;
        if (z < dist_ + 1.f) continue;
        for (int side = -1; side <= 1; side += 2) {
            const float wx = centerAt(z) + float(side) * kWall;
            const Proj floor = project(wx, 0.f, z);
            const Proj top = project(wx, 3.25f, z);
            if (!floor.ok || !top.ok) continue;
            const float h = std::fabs(floor.y - top.y);
            const float cy = (floor.y + top.y) * 0.5f;
            const float w = std::max(4.f, 0.62f * floor.s);
            quad(art_.wall, floor.x, cy, w, h, PAL_WALL, floor.fog, side < 0);
            if ((i & 1) == 0) {
                const Proj lamp = project(wx - float(side) * 0.15f, 2.55f, z);
                if (lamp.ok) quad(art_.lamp, lamp.x, lamp.y, 0.28f * lamp.s, 0.28f * lamp.s, PAL_LAMP, lamp.fog);
            }
        }
    }

    if (mode_ == Mode::Title) {
        const float lh = 30.f;
        quad(art_.mark, 160, 16, lh * float(art_.mark.w) / float(art_.mark.h), lh * 0.55f, PAL_LOGO, 0);
        quad(art_.logo, 160, 40, 34.f * float(art_.logo.w) / float(art_.logo.h), 34.f, PAL_LOGO, 0);
    }

    const bool low = mode_ == Mode::Run && rounds() <= 12;
    const int rpal = low ? PAL_BAD : PAL_AMBER;
    const int shown = mode_ == Mode::Title ? int(kAmmo0) : rounds();
    const float dh = mode_ == Mode::Title ? 16.f : 26.f;
    const float dy = mode_ == Mode::Title ? 62.f : 16.f;
    if (shown >= 10) {
        const float dw = dh * float(art_.digit[0].w) / float(art_.digit[0].h);
        quad(art_.digit[shown / 10], 160.f - dw * 0.55f, dy, dw, dh, rpal, 0);
        quad(art_.digit[shown % 10], 160.f + dw * 0.55f, dy, dw, dh, rpal, 0);
    } else {
        const float dw = dh * float(art_.digit[0].w) / float(art_.digit[0].h);
        quad(art_.digit[std::max(0, shown)], 160.f, dy, dw, dh, rpal, 0);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hud(1, 0, "S3 BLITZ", PAL_AMBER);
        hud(30, 0, "TRENCH", PAL_HUD);
        hudC(23, "THE MAGAZINE IS THE CLOCK", PAL_AMBER);
        hudC(24, "ARROWS BANK AND CLIMB", PAL_HUD);
        hudC(25, "Z/C/SPACE FIRES", PAL_HUD);
        hudC(26, "HIT TOWERS TO PUT ROUNDS BACK", PAL_GOOD);
        if (((sys_->frame / 20) & 1) == 0) hudC(27, "ENTER RUNS", PAL_AMBER);
        return;
    }

    hud(1, 0, "S3 BLITZ", PAL_AMBER);
    const int pct = clampi(int(dist_ / kPortZ * 100.f), 0, 100);
    std::snprintf(buf, sizeof buf, "RUN %d%%", pct);
    hud(32, 0, buf, PAL_HUD);
    const char* band = alt_ > 0.78f ? "HIGH" : alt_ < 0.24f ? "LOW" : "MID";
    std::snprintf(buf, sizeof buf, "ALT %s", band);
    hud(1, 4, buf, PAL_HUD);
    if (mode_ == Mode::Run) {
        if (const char* h = hint()) hudC(4, h, low ? PAL_BAD : PAL_AMBER);
        if (low && ((sys_->frame / 10) & 1)) hud(28, 4, "LOW", PAL_BAD);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_AMBER);
        hudC(14, "ENTER CONTINUES", PAL_HUD);
        hudC(15, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Win) {
        hudC(12, "PORT OPEN", PAL_GOOD);
        std::snprintf(buf, sizeof buf, "ROUNDS LEFT %d", rounds());
        hudC(14, buf, PAL_AMBER);
        std::snprintf(buf, sizeof buf, "TOWERS %d   CELLS %d", towers_, cells_);
        hudC(15, buf, PAL_HUD);
        if (!bot_) hudC(17, "ENTER RUNS AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Dead) {
        const char* line = "HULL BROKEN";
        if (!std::strcmp(note_, "WALL")) line = "INTO THE WALL";
        else if (!std::strcmp(note_, "DRY")) line = "MAGAZINE DRY";
        else if (!std::strcmp(note_, "MOUTH")) line = "MISSED THE MOUTH";
        hudC(12, line, PAL_BAD);
        std::snprintf(buf, sizeof buf, "ROUNDS %d   TOWERS %d", rounds(), towers_);
        hudC(14, buf, PAL_HUD);
        if (!bot_) hudC(16, "ENTER RUNS AGAIN", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    flash_ = std::max(0.f, flash_ - kDt * 1.6f);
    shake_ = std::max(0.f, shake_ - kDt * 1.8f);
    shotSnd_ = std::max(0.f, shotSnd_ - kDt);
    tickSnd_ = std::max(0.f, tickSnd_ - kDt);
    for (Boom& b : booms_) b.t += kDt;
    booms_.erase(std::remove_if(booms_.begin(), booms_.end(), [](const Boom& b) { return b.t > 0.42f; }), booms_.end());

    if (mode_ == Mode::Title) {
        t_ += kDt;
        dist_ = 48.f + 32.f * std::sin(t_ * 0.35f);
        localX_ = 0.8f * std::sin(t_ * 0.8f);
        vx_ = 0.8f * 0.8f * std::cos(t_ * 0.8f) * 10.f;
        alt_ = 0.5f;
        const bool go = (bot_ && t_ > 0.35f) || (!bot_ && sys.pad.pressed(gs::BTN_START));
        if (!go) {
            draw();
            audio();
            return;
        }
        newRun();
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (sys.pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            t_ = 0;
            over_ = false;
        }
    } else if (mode_ == Mode::Run) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
        } else {
            const int before = rounds();
            update();
            if (mode_ == Mode::Run && lastShown_ >= 0 && rounds() < lastShown_) tickSnd_ = 0.04f;
            lastShown_ = rounds();
            if (before < 0) lastShown_ = rounds();
        }
    } else {
        t_ += kDt;
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) newRun();
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            t_ = 0;
            over_ = false;
            won_ = false;
        }
    }

    if (mode_ == Mode::Run && sys_ && !sys_->headless) {
        const int heat = clampi(rounds(), 0, 48);
        sys.setLight(180 - heat * 2, 40 + heat, 20);
    }
    draw();
    audio();
}

}  // namespace blitz

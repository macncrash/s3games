#include "logs.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace logs {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float HORIZON = 68.f;
constexpr float SPAN = 156.f;
constexpr float Z_NEAR = 5.0f;
constexpr float PPM_NEAR = 21.f;
constexpr float CAM_BACK = 7.0f;
constexpr float RIVER_HALF = 5.2f;
constexpr float BOAT_LIM = 2.55f;
constexpr float ROCK_OFF = 4.55f;
constexpr float GAP = 3.25f;
constexpr float FOLLOW = 0.34f;
constexpr float BEND_A = 3.5f;
constexpr float BEND_W = 0.0305f;
constexpr float CURRENT = 8.0f;
constexpr float CRUISE = 10.4f;
constexpr float BOOST = 13.4f;
constexpr float EASE = 6.5f;
constexpr float STEER_V = 8.2f;
constexpr float BOOM_Z = 460.f;

float bend(float z) { return BEND_A * std::sin(BEND_W * z); }
float bendD(float z) { return BEND_A * BEND_W * std::cos(BEND_W * z); }

uint32_t hashZ(float z) {
    uint32_t n = uint32_t(std::floor(z * 3.f)) * 747796405u + 2891336453u;
    n ^= n >> 16;
    n *= 0x7feb352du;
    return n;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    return 1;
}

int Game::meters() const {
    float left = BOOM_Z - boatZ_;
    if (left < 0) left = 0;
    return int(left + 0.5f);
}

int Game::afloat() const {
    int n = 0;
    for (auto& s : logs_)
        if (s.st == St::Afloat) n++;
    return n;
}

int Game::delivered() const {
    int n = 0;
    for (auto& s : logs_)
        if (s.st == St::In) n++;
    return n;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.25f, 0.12f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    t_ = 0;
    scenery_ = 28.f;
    race_ = 0;
    hull_ = 3;
    fan_ = -2;
    failTone_ = 0;
    flashT_ = 0;
    flash_ = nullptr;
    report_[0] = 0;
    pole_ = false;
    for (auto& p : pops_) p.life = 0;
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    race_ = 0;
    hull_ = 3;
    hit_ = 0;
    vx_ = 0;
    why_ = Why::None;
    fan_ = -2;
    failTone_ = 0;
    knock_ = 0;
    chime_ = 0;
    flashT_ = 0;
    flash_ = nullptr;
    report_[0] = 0;
    boatZ_ = 12.f;
    boatX_ = bend(boatZ_);
    for (int i = 0; i < LOG_N; i++) {
        Stick& s = logs_[i];
        s.st = St::Afloat;
        s.kind = i % 3;
        s.z = 18.f + float(i) * 3.05f;
        s.x = bend(s.z) + float((i % 3) - 1) * 0.62f;
        s.vx = bendD(s.z) * CURRENT * 0.75f;
        s.knock = 0;
    }
    hazards_.clear();
    const float rocks[] = {110.f, 168.f, 226.f, 286.f, 344.f, 402.f};
    for (float z : rocks) {
        float side = std::cos(BEND_W * z) < 0.f ? 1.f : -1.f;
        hazards_.push_back({z, side * ROCK_OFF, 0});
    }
    const float snags[] = {142.f, 254.f, 372.f};
    for (float z : snags) {
        float side = std::cos(BEND_W * z) < 0.f ? 1.f : -1.f;
        hazards_.push_back({z, side * (ROCK_OFF + 0.15f), 1});
    }
    for (auto& p : pops_) p.life = 0;
}

void Game::splash(float x, float z, int kind) {
    for (auto& p : pops_) {
        if (p.life > 0) continue;
        p.x = x;
        p.z = z;
        p.life = kind == 0 ? 0.38f : 0.55f;
        p.kind = kind;
        return;
    }
}

void Game::finish(bool win, Why why) {
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return;
    mode_ = win ? Mode::Won : Mode::Lost;
    won_ = win;
    over_ = true;
    why_ = why;
    int in = delivered();
    if (win) {
        std::snprintf(report_, sizeof report_,
                      "S3 LOGS  PASS  the drive reached the boom  %d of %d sticks  %.1f s", in, LOG_N, race_);
        fan_ = 0;
        fanT_ = 0;
    } else if (why == Why::Stove) {
        std::snprintf(report_, sizeof report_,
                      "S3 LOGS  FAIL  the bateau stove in  %d of %d sticks  %.1f s", in, LOG_N, race_);
        failTone_ = 0.7f;
    } else {
        std::snprintf(report_, sizeof report_,
                      "S3 LOGS  FAIL  the drive broke before the boom  %d of %d sticks  %.1f s", in, LOG_N, race_);
        failTone_ = 0.7f;
    }
}

void Game::assist() {
    steer_ = 0;
    pole_ = false;
    ease_ = false;
    boost_ = false;
    int worst = -1;
    float worstAbs = 0.f;
    float tail = 1e9f, lead = -1.f;
    int n = 0;
    for (int i = 0; i < LOG_N; i++) {
        if (logs_[i].st != St::Afloat) continue;
        n++;
        tail = std::min(tail, logs_[i].z);
        lead = std::max(lead, logs_[i].z);
        float off = std::fabs(logs_[i].x - bend(logs_[i].z));
        if (worst < 0 || off > worstAbs) {
            worstAbs = off;
            worst = i;
        }
    }
    if (!n || worst < 0) return;

    float off = logs_[worst].x - bend(logs_[worst].z);
    float side = std::fabs(off) >= 0.35f ? (off >= 0.f ? 1.f : -1.f) : (-bendD(boatZ_) >= 0.f ? 1.f : -1.f);
    // Sit just outside the worst stick, but never in the rocks.
    float mag = std::clamp(std::fabs(off) + 0.95f, 1.15f, BOAT_LIM);
    float targetOff = side * mag;
    float goalZ = worstAbs > 0.5f ? logs_[worst].z : 0.5f * (tail + lead);
    float dz = goalZ - boatZ_;
    if (dz > 1.6f) boost_ = true;
    else if (dz < -0.25f) ease_ = true;
    pole_ = worstAbs > 0.4f && std::fabs(dz) < 6.f;
    for (auto& h : hazards_) {
        float hz = h.z - boatZ_;
        if (hz < -1.5f || hz > 10.f) continue;
        if (std::fabs(targetOff - h.off) < 1.9f) targetOff = h.off > 0.f ? h.off - 2.2f : h.off + 2.2f;
    }
    targetOff = std::clamp(targetOff, -BOAT_LIM, BOAT_LIM);
    float err = (bend(boatZ_) + targetOff) - boatX_;
    steer_ = std::clamp(err * 1.05f, -1.f, 1.f);
}

void Game::update(float dt) {
    race_ += dt;
    if (hit_ > 0) hit_ -= dt;
    if (flashT_ > 0) flashT_ -= dt;
    for (auto& s : logs_)
        if (s.knock > 0) s.knock -= dt;
    for (auto& p : pops_)
        if (p.life > 0) p.life -= dt;

    if (bot_) assist();
    else {
        const gs::Pad& p = sys_->pad;
        steer_ = 0;
        if (p.down(gs::BTN_LEFT)) steer_ -= 1.f;
        if (p.down(gs::BTN_RIGHT)) steer_ += 1.f;
        if (std::fabs(p.axisX) > 0.18f) steer_ = p.axisX;
        steer_ = std::clamp(steer_, -1.f, 1.f);
        pole_ = p.down(gs::BTN_C);
        ease_ = p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.down(gs::BTN_DOWN);
        boost_ = pole_ && !ease_;
    }

    float spd = ease_ ? EASE : (boost_ ? BOOST : CRUISE);
    boatZ_ += spd * dt;
    if (boatZ_ > BOOM_Z - 1.6f) boatZ_ = BOOM_Z - 1.6f;
    float want = steer_ * STEER_V;
    float step = std::clamp(want - vx_, -24.f * dt, 24.f * dt);
    vx_ += step;
    boatX_ += vx_ * dt;
    float boff = boatX_ - bend(boatZ_);
    if (boff > BOAT_LIM) {
        boatX_ = bend(boatZ_) + BOAT_LIM;
        vx_ = std::min(vx_, 0.f);
    } else if (boff < -BOAT_LIM) {
        boatX_ = bend(boatZ_) - BOAT_LIM;
        vx_ = std::max(vx_, 0.f);
    }

    float follow = race_ < 2.4f ? 0.78f : FOLLOW;
    for (auto& s : logs_) {
        if (s.st == St::In) {
            float slot = float((s.kind * 2) - 2) * 0.55f;
            float off = s.x - bend(s.z);
            s.x += (slot - off) * std::min(1.f, 1.6f * dt);
            if (s.z < BOOM_Z + 16.f) s.z += CURRENT * 0.42f * dt;
            continue;
        }
        if (s.st != St::Afloat) continue;
        s.z += CURRENT * dt;
        float fv = bendD(s.z) * CURRENT * follow;
        s.vx += (fv - s.vx) * std::min(1.f, 3.5f * dt);
        s.x += s.vx * dt;
    }

    for (int i = 0; i < LOG_N; i++) {
        for (int j = i + 1; j < LOG_N; j++) {
            if (logs_[i].st != St::Afloat || logs_[j].st != St::Afloat) continue;
            float dx = logs_[j].x - logs_[i].x;
            float dz = logs_[j].z - logs_[i].z;
            if (std::fabs(dz) < 2.3f && std::fabs(dx) < 0.9f) {
                float push = (dx >= 0.f ? 1.f : -1.f) * 0.7f * dt;
                logs_[j].x += push;
                logs_[i].x -= push;
            }
        }
    }

    bool bumped = false;
    for (auto& s : logs_) {
        if (s.st != St::Afloat) continue;
        float off = s.x - bend(s.z);
        float boatOff = boatX_ - bend(boatZ_);
        float dx = s.x - boatX_;
        float dz = s.z - boatZ_;
        float xReach = pole_ ? 3.05f : 1.55f;
        float zBack = pole_ ? 3.6f : 1.3f;
        float zFore = pole_ ? 4.4f : 2.1f;
        if (dz >= -zBack && dz <= zFore && std::fabs(dx) <= xReach && std::fabs(off) >= 0.22f) {
            bool outside = std::fabs(boatOff) > std::fabs(off) + 0.02f && boatOff * off > 0.f;
            float power = 0.f;
            if (outside && pole_) power = 12.f;
            else if (outside) power = 2.6f;
            else if (pole_ && std::fabs(off) > 0.8f) power = 6.5f;
            if (power > 0.f) {
                float dir = off > 0.f ? -1.f : 1.f;
                s.x += dir * power * dt;
                s.vx = bendD(s.z) * CURRENT * follow + dir * 1.1f;
                if (s.knock <= 0.f) {
                    s.knock = 0.16f;
                    bumped = true;
                    splash(s.x, s.z, 0);
                }
            }
        }
    }
    if (bumped) knock_ = std::max(knock_, pole_ ? 0.22f : 0.1f);

    auto lose = [&](Stick& s, Why why, const char* word) {
        if (s.st != St::Afloat) return;
        s.st = St::Lost;
        why_ = why;
        flash_ = word;
        flashT_ = 1.25f;
        splash(s.x, s.z, 1);
        knock_ = std::max(knock_, 0.16f);
    };

    for (auto& s : logs_) {
        if (s.st != St::Afloat) continue;
        float off = s.x - bend(s.z);
        if (std::fabs(off) > RIVER_HALF - 0.12f) {
            lose(s, Why::Beach, "BEACHED");
            continue;
        }
        for (auto& h : hazards_) {
            float dx = s.x - (bend(h.z) + h.off);
            float dz = s.z - h.z;
            float rx = h.kind == 0 ? 1.22f : 1.35f;
            float rz = h.kind == 0 ? 1.55f : 2.5f;
            if (std::fabs(dx) < rx && std::fabs(dz) < rz) {
                lose(s, h.kind == 0 ? Why::Deadhead : Why::Snag, h.kind == 0 ? "DEADHEAD" : "SNAG");
                break;
            }
        }
        if (s.st != St::Afloat) continue;
        if (s.z >= BOOM_Z) {
            float gate = s.x - bend(BOOM_Z);
            if (std::fabs(gate) <= GAP) {
                s.st = St::In;
                chime_ = 0.14f;
                chimeF_ = 720.f + float(delivered()) * 40.f;
                splash(s.x, s.z, 0);
            } else {
                lose(s, Why::Jam, "JAMMED");
            }
        }
    }

    if (hit_ <= 0.f) {
        for (auto& h : hazards_) {
            float dx = boatX_ - (bend(h.z) + h.off);
            float dz = boatZ_ - h.z;
            float r = h.kind == 0 ? 1.35f : 1.55f;
            if (dx * dx + dz * dz < r * r) {
                hull_--;
                hit_ = 1.35f;
                boatX_ += (dx >= 0.f ? 1.f : -1.f) * 0.45f;
                knock_ = 0.3f;
                sys_->rumble(0.45f, 0.7f, 90);
                flash_ = "HULL";
                flashT_ = 1.1f;
                break;
            }
        }
    }

    int in = delivered();
    int live = afloat();
    int lost = LOG_N - in - live;
    if (hull_ <= 0) finish(false, Why::Stove);
    else if (lost > LOG_N - NEED) finish(false, Why::Beach);
    else if (live == 0) finish(in >= NEED, in >= NEED ? Why::None : Why::Jam);
    else if (race_ > 100.f) finish(false, Why::Jam);
}

Game::Proj Game::project(float wx, float wz) const {
    Proj p;
    float d = wz - cam_;
    if (d < 4.4f) return p;
    float t = Z_NEAR / d;
    p.y = HORIZON + t * SPAN;
    if (p.y < HORIZON || p.y > gs::SCREEN_H + 48.f) return p;
    p.ppm = PPM_NEAR * t;
    p.x = 160.f + (wx - bend(cam_)) * p.ppm;
    p.ok = true;
    return p;
}

int Game::fogFor(float wz) const {
    float d = wz - cam_;
    if (d < 10.f) return 0;
    float t = Z_NEAR / std::max(d, 1.f);
    float f = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
    return int(f * 13.f);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (!(h > 1.5f) || m.h <= 0) return;
    const gs::Image& img = m.pick(h);
    float s = h / float(img.h);
    gs::Sprite sp;
    sp.img = img;
    sp.h = std::max(1, int(std::lround(h)));
    sp.w = std::max(1, int(std::lround(img.w * s)));
    sp.x = int(std::lround(cx - sp.w * 0.5f));
    sp.y = int(std::lround(cy - sp.h * 0.5f));
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sp.fog = uint8_t(std::clamp(fog, 0, 16));
    sp.shadow = shadow;
    sys_->vdp.sprite(sp);
}

void Game::river() {
    gs::VDP& v = sys_->vdp;
    float camBend = bend(cam_);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y <= int(HORIZON)) {
            v.road[y].on = false;
            float u = float(y) / HORIZON;
            int r = std::clamp(int(2 + u * 9), 0, 15);
            int g = std::clamp(int(4 + u * 8), 0, 15);
            int b = std::clamp(int(10 + u * 3), 0, 15);
            if (y > int(HORIZON) - 8) {
                r = std::min(15, r + 2);
                g = std::min(15, g + 1);
            }
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) - HORIZON) / SPAN;
        t = std::max(t, 0.012f);
        float d = Z_NEAR / t;
        float worldZ = cam_ + d;
        float ppm = PPM_NEAR * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + (bend(worldZ) - camBend) * ppm;
        rd.hw = std::max(2.f, RIVER_HALF * ppm);
        rd.v = worldZ * 42.f;
        rd.pal = PAL_ROAD;
        rd.style = 2;
        rd.band = (int(std::floor(worldZ * 0.45f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 12.f);
        v.lineBackdrop[y] = gs::rgb4(1, 4, 7);
    }
}

void Game::text(int col, int row, const char* s, int pal) {
    if (!s) return;
    for (int i = 0; s[i]; i++, col++) {
        unsigned char c = (unsigned char)s[i];
        if (c >= 'a' && c <= 'z') c = (unsigned char)(c - 32);
        if (c < 32 || c > 127 || col < 0 || col > 39 || row < 0 || row > 27) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(col, row, gs::entry(tile, pal));
    }
}

void Game::textC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    text((40 - n) / 2, row, s, pal);
}

void Game::boatAt(float wx, float wz, bool pole, bool flip) {
    Proj p = project(wx, wz);
    if (!p.ok) return;
    float bob = std::sin(t_ * 4.2f) * 1.4f;
    float shake = hit_ > 0.f ? std::sin(t_ * 48.f) * 4.f * std::min(hit_, 0.6f) : 0.f;
    float h = std::clamp(p.ppm * 3.15f, 8.f, 78.f);
    float y = p.y - h * 0.18f + bob;
    spr(art_.shadow, p.x + shake, p.y + h * 0.28f, h * 0.28f, PAL_FX, false, 0, true);
    spr(art_.splash, p.x + shake, p.y + h * 0.22f, h * 0.22f, PAL_FX, flip, 0);
    spr(art_.boat[pole ? 1 : 0], p.x + shake, y, h, PAL_BOAT, flip, fogFor(wz));
}

void Game::world() {
    auto drawStick = [&](const Stick& s, int i) {
        if (s.st == St::Lost) return;
        Proj p = project(s.x, s.z);
        if (!p.ok) return;
        float bob = std::sin(t_ * 2.4f + float(i) * 0.7f) * p.ppm * 0.06f;
        float h = std::clamp(p.ppm * 2.7f, 3.f, 64.f);
        int fog = fogFor(s.z);
        spr(art_.shadow, p.x, p.y + h * 0.22f, h * 0.22f, PAL_FX, false, fog, true);
        spr(art_.stick[s.kind], p.x, p.y - h * 0.28f + bob, h, PAL_LOG, (i & 1) != 0, fog);
    };

    int ord[LOG_N];
    for (int i = 0; i < LOG_N; i++) ord[i] = i;
    std::sort(ord, ord + LOG_N, [&](int a, int b) { return logs_[a].z < logs_[b].z; });
    for (int k = 0; k < LOG_N; k++) drawStick(logs_[ord[k]], ord[k]);

    for (auto& pop : pops_) {
        if (pop.life <= 0) continue;
        Proj p = project(pop.x, pop.z);
        if (!p.ok) continue;
        float h = (pop.kind == 0 ? 10.f : 16.f) + (0.5f - pop.life) * 8.f;
        spr(art_.splash, p.x, p.y, h, PAL_FX);
    }

    for (auto& h : hazards_) {
        Proj p = project(bend(h.z) + h.off, h.z);
        if (!p.ok) continue;
        int fog = fogFor(h.z);
        if (h.kind == 0) {
            float hs = std::clamp(p.ppm * 1.7f, 3.f, 40.f);
            spr(art_.rock, p.x, p.y - hs * 0.2f, hs, PAL_ROCK, h.off < 0, fog);
        } else {
            float hs = std::clamp(p.ppm * 1.15f, 3.f, 28.f);
            spr(art_.sweeper, p.x, p.y - hs * 0.15f, hs, PAL_LOG, h.off < 0, fog);
        }
    }

    // Sorting boom: pilings, chained wings, the gap the drive has to clear.
    {
        float z = BOOM_Z;
        float posts[] = {-RIVER_HALF + 0.35f, -GAP, GAP, RIVER_HALF - 0.35f};
        for (float off : posts) {
            Proj p = project(bend(z) + off, z);
            if (!p.ok) continue;
            float h = std::clamp(p.ppm * 3.4f, 4.f, 90.f);
            spr(art_.piling, p.x, p.y - h * 0.42f, h, PAL_BOOM, false, fogFor(z));
        }
        float wings[] = {-(GAP + RIVER_HALF) * 0.5f, (GAP + RIVER_HALF) * 0.5f};
        for (float off : wings) {
            Proj p = project(bend(z) + off, z + 0.4f);
            if (!p.ok) continue;
            float h = std::clamp(p.ppm * 1.15f, 3.f, 36.f);
            spr(art_.wing, p.x, p.y - h * 0.2f, h, PAL_BOOM, off > 0, fogFor(z));
        }
        Proj sign = project(bend(z), z);
        if (sign.ok) {
            float h = std::clamp(sign.ppm * 1.7f, 6.f, 28.f);
            spr(art_.sign, sign.x, sign.y - sign.ppm * 3.6f, h, PAL_AMBER, false, fogFor(z));
        }
        Proj mill = project(bend(z + 8.f) + RIVER_HALF + 2.3f, z + 8.f);
        if (mill.ok) {
            float h = std::clamp(mill.ppm * 4.2f, 6.f, 96.f);
            int frame = int(t_ * 6.f) & 1;
            spr(art_.mill[frame], mill.x, mill.y - h * 0.4f, h, PAL_BOOM, false, fogFor(z + 8.f));
        }
    }

    float z0 = std::floor((cam_ - 4.f) / 13.f) * 13.f;
    for (float z = z0; z < cam_ + 78.f; z += 13.f) {
        if (z < 4.f) continue;
        if (std::fabs(z - BOOM_Z) < 10.f) continue;
        uint32_t hsh = hashZ(z);
        for (int side = 0; side < 2; side++) {
            float sign = side == 0 ? -1.f : 1.f;
            float off = sign * (RIVER_HALF + 0.9f + float(hsh % 5) * 0.22f);
            Proj p = project(bend(z) + off, z);
            if (!p.ok) continue;
            float h = std::clamp(p.ppm * (4.2f + float((hsh >> 3) % 3) * 0.35f), 3.f, 86.f);
            int kind = (hsh >> (side + 1)) & 1;
            spr(art_.tree[kind], p.x, p.y - h * 0.46f, h, PAL_TREE, side == 1, fogFor(z));
        }
        if ((hsh % 7) == 0 && z > 30.f && z < BOOM_Z - 20.f) {
            float sign = (hsh & 2) ? 1.f : -1.f;
            Proj p = project(bend(z) + sign * (RIVER_HALF + 2.4f), z);
            if (!p.ok) continue;
            float h = std::clamp(p.ppm * 3.3f, 4.f, 70.f);
            spr(art_.cabin, p.x, p.y - h * 0.42f, h, PAL_BOOM, sign > 0, fogFor(z));
        }
    }
}

void Game::skyDress() {
    spr(art_.sun, 286, 18, 16, PAL_SKY);
    float drift = t_ * 6.f;
    spr(art_.cloud, std::fmod(20.f + drift, 380.f) - 30.f, 14, 16, PAL_SKY);
    spr(art_.cloud, std::fmod(200.f + drift * 0.65f, 400.f) - 40.f, 30, 20, PAL_SKY);
    for (int i = 0; i < 3; i++) {
        float x = std::fmod(30.f + float(i) * 110.f + t_ * (16.f + float(i) * 7.f), 370.f) - 20.f;
        spr(art_.bird, x, 42.f + float(i) * 8.f, 6.f + float(i == 2), PAL_SKY, (i & 1) != 0);
    }
}

void Game::hud() {
    if (mode_ == Mode::Title) {
        textC(10, "GET OUTSIDE THE STICK", PAL_AMBER);
        textC(11, "C POLES IT IN", PAL_AMBER);
        textC(13, "ARROWS STEER   X EASES", PAL_HUD);
        if (int(t_ * 2.f) & 1) textC(15, "START", PAL_GOOD);
        return;
    }
    if (mode_ == Mode::Pause) {
        textC(12, "PAUSED", PAL_AMBER);
        return;
    }
    char left[32], right[16];
    std::snprintf(left, sizeof left, "STICKS %d  IN %d  H%d", afloat(), delivered(), hull_);
    int m = meters();
    if (mode_ == Mode::Won) std::snprintf(right, sizeof right, "BOOM");
    else std::snprintf(right, sizeof right, "%d M", m);
    text(1, 0, left, PAL_HUD);
    text(39 - int(std::strlen(right)), 0, right, mode_ == Mode::Won ? PAL_GOOD : PAL_HUD);

    float off = boatX_ - bend(std::max(boatZ_, cam_));
    char bar[20];
    const int n = 17;
    int mid = n / 2;
    int pos = mid + int(std::lround((off / RIVER_HALF) * float(mid - 1)));
    pos = std::clamp(pos, 0, n - 1);
    for (int i = 0; i < n; i++) bar[i] = '-';
    bar[0] = '<';
    bar[n - 1] = '>';
    int mark = -1;
    float worst = 0.7f;
    for (auto& s : logs_) {
        if (s.st != St::Afloat) continue;
        float o = s.x - bend(s.z);
        if (std::fabs(o) > worst) {
            worst = std::fabs(o);
            mark = mid + int(std::lround((o / RIVER_HALF) * float(mid - 1)));
            mark = std::clamp(mark, 0, n - 1);
        }
    }
    if (mark >= 0 && mark != pos) bar[mark] = 'o';
    bar[pos] = '+';
    bar[n] = 0;
    int pal = std::fabs(off) > BOAT_LIM * 0.82f || worst > 3.1f ? PAL_ALERT : PAL_HUD;
    textC(1, bar, pal);

    float behind = 0;
    for (auto& s : logs_) {
        if (s.st != St::Afloat) continue;
        float dz = boatZ_ - s.z;
        if (dz > behind) behind = dz;
    }
    if (flashT_ > 0.f && flash_) textC(2, flash_, PAL_ALERT);
    else if (mode_ == Mode::Lost) textC(2, "THE DRIVE BROKE", PAL_ALERT);
    else if (mode_ == Mode::Won) textC(2, "THE DRIVE IS IN", PAL_GOOD);
    else if (mode_ == Mode::Play && behind > 5.f && worst > 1.4f) textC(2, "DROP BACK  X", PAL_ALERT);
    else if (mode_ == Mode::Play && worst > 2.4f) textC(2, "POLE IT IN", PAL_AMBER);
    else if (mode_ == Mode::Play && race_ < 4.f) textC(2, "KEEP THE DRIVE IN THE CHANNEL", PAL_DIM);
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    if (mode_ == Mode::Title) cam_ = scenery_;
    else {
        cam_ = boatZ_ - CAM_BACK;
        float lock = BOOM_Z - 16.f;
        if (cam_ > lock) cam_ = lock;
    }
    river();

    if (mode_ == Mode::Title) {
        spr(art_.title, 160, 22, float(art_.title.h), PAL_TITLE);
        spr(art_.line1, 160, 48, float(art_.line1.h), PAL_TITLE);
        spr(art_.line2, 160, 68, float(art_.line2.h), PAL_AMBER);
        bool showPole = (int(t_ * 2.2f) % 3) == 0;
        float bz = scenery_ + CAM_BACK;
        boatAt(bend(bz), bz, showPole, std::sin(t_ * 0.8f) > 0.2f);
        for (int i = 0; i < 7; i++) {
            float z = scenery_ + 14.f + float(i) * 3.3f;
            float off = std::sin(t_ * 0.65f + float(i) * 0.8f) * 1.15f;
            Proj p = project(bend(z) + off, z);
            if (!p.ok) continue;
            float h = std::clamp(p.ppm * 2.7f, 3.f, 60.f);
            spr(art_.stick[i % 3], p.x, p.y - h * 0.28f, h, PAL_LOG, (i & 1) != 0, fogFor(z));
        }
        float z0 = std::floor(scenery_ / 13.f) * 13.f;
        for (float z = z0; z < scenery_ + 70.f; z += 13.f) {
            uint32_t hsh = hashZ(z);
            for (int side = 0; side < 2; side++) {
                float sign = side ? 1.f : -1.f;
                Proj p = project(bend(z) + sign * (RIVER_HALF + 1.3f), z);
                if (!p.ok) continue;
                float h = std::clamp(p.ppm * 4.4f, 3.f, 80.f);
                spr(art_.tree[hsh & 1], p.x, p.y - h * 0.46f, h, PAL_TREE, side, fogFor(z));
            }
        }
    } else {
        if (mode_ == Mode::Won) spr(art_.win, 160, 40, float(art_.win.h), PAL_GOOD);
        else if (mode_ == Mode::Lost) spr(art_.fail, 160, 40, float(art_.fail.h), PAL_ALERT);
        float best = 1e9f;
        float lookX = boatX_;
        for (auto& s : logs_) {
            if (s.st != St::Afloat) continue;
            float dz = std::fabs(s.z - boatZ_);
            if (dz < best) {
                best = dz;
                lookX = s.x;
            }
        }
        bool flip = lookX < boatX_;
        boatAt(boatX_, boatZ_, pole_, flip);
        world();
    }
    skyDress();
    hud();
}

void Game::audio() {
    float bed = mode_ == Mode::Title ? 0.012f : 0.02f;
    sys_->apu.noise(bed, 640.f, false);
    knock_ *= 0.86f;
    sys_->apu.tone(1, pole_ ? 168.f : 96.f, knock_);
    if (chime_ > 0.f) {
        chime_ -= DT;
        sys_->apu.tone(2, chimeF_, 0.07f);
    } else if (failTone_ > 0.f) {
        failTone_ -= DT;
        sys_->apu.tone(2, 90.f, 0.06f * failTone_);
    } else if (fan_ >= 0 && fan_ < 4) {
        static const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
        fanT_ += DT;
        sys_->apu.tone(0, notes[fan_], 0.07f);
        sys_->apu.tone(2, notes[std::min(fan_, 2)] * 0.5f, 0.04f);
        if (fanT_ > 0.14f) {
            fanT_ = 0;
            fan_++;
        }
    } else if (fan_ >= 4) {
        sys_->apu.tone(0, 523.25f, 0.045f);
        sys_->apu.tone(2, 783.99f, 0.03f);
    } else {
        float drone = mode_ == Mode::Play ? 0.018f : 0.f;
        sys_->apu.tone(0, 62.f + (boost_ ? 14.f : 0.f), drone);
        sys_->apu.tone(2, 0.f, 0.f);
    }
    if (mode_ == Mode::Play && flashT_ > 0.8f) sys_->setLight(180, 40, 30);
    else if (mode_ == Mode::Won) sys_->setLight(40, 160, 70);
    else if (mode_ == Mode::Play) sys_->setLight(30, 80, 140);
    else sys_->setLight(40, 50, 90);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        scenery_ += DT * 7.5f;
        if (pad.pressed(gs::BTN_START) || (bot_ && t_ > 0.5f)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE) && !bot_) toTitle();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) toTitle();
        else update(DT);
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
        toTitle();
    }
    draw();
    audio();
}

}  // namespace logs

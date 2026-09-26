#include "game/bann.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rbann {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kWatch = 40.f;
constexpr float kBannerZ = 28.f;
constexpr float kHorizon = 70.f;
constexpr float kZScale = 255.f;
constexpr float kZLine = 2.4f;
constexpr float kEdge = 1.08f;
constexpr float kOut = 6.8f;
constexpr float kBack = 6.0f;
constexpr float kSide = 3.4f;
constexpr float kDodge = 0.92f;

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 4;
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return 0;
    if (!carrying_) return 1;
    if (pz_ > 16.f) return 2;
    return 3;
}

float Game::windNow() const {
    if (mode_ != Mode::Play) return 0.f;
    auto hump = [&](float center, float amp, float wid) {
        float d = std::fabs(watch_ - center);
        if (d >= wid) return 0.f;
        float x = 1.f - d / wid;
        return amp * x * x;
    };
    return hump(2.6f, 0.95f, 0.62f) + hump(8.4f, -1.05f, 0.66f) + hump(14.8f, 0.9f, 0.6f) +
           hump(22.f, -0.85f, 0.62f) + hump(30.f, 1.0f, 0.6f);
}

float Game::foeU(const Foe& f) const { return f.lane + std::sin(t_ * 1.25f + f.phase) * 0.05f; }

float Game::viewZ(float worldZ) const { return kZLine + (worldZ - pz_) * float(face_); }

float Game::bendAt(float row) const {
    return std::sin(row * 0.017f + pz_ * 0.13f) * (6.f + row * 0.035f);
}

float Game::halfAt(float row) const {
    float pinch = 1.f - 0.12f * std::sin(pz_ * 0.41f + row * 0.01f);
    return (22.f + row * 0.58f) * pinch;
}

int Game::horizon() const { return std::clamp(int(std::lround(kHorizon + shy_)), 48, 96); }

Game::Spot Game::spot(float u, float vz, float base) const {
    Spot s;
    if (!(vz > 0.95f) || vz > 46.f) return s;
    float row = kZScale / vz;
    float feet = kZScale / kZLine;
    float t = std::clamp(row / feet, 0.02f, 1.45f);
    s.h = std::max(5.f, base * std::pow(t, 0.72f));
    float along = std::clamp((vz - kZLine) / 34.f, 0.f, 1.f);
    float lift = std::pow(along, 1.18f) * s.h * 0.62f;
    s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
    s.y = float(horizon()) + row - lift + shy_;
    s.fog = int(std::clamp(along * 9.f, 0.f, 9.f));
    s.ok = true;
    return s;
}

void Game::layout() {
    foes_.clear();
    props_.clear();
    puffs_.clear();
    auto foe = [&](float lo, float hi, float lane, float phase, int dir) {
        Foe f;
        f.lo = lo;
        f.hi = hi;
        f.z = (lo + hi) * 0.5f;
        f.lane = lane;
        f.phase = phase;
        f.dir = dir;
        f.on = true;
        foes_.push_back(f);
    };
    foe(6.f, 9.2f, -0.34f, 0.4f, -1);
    foe(12.6f, 16.4f, 0.36f, 1.7f, 1);
    foe(19.4f, 22.4f, -0.32f, 2.8f, -1);

    auto prop = [&](float z, float u, float h, int kind) {
        Prop p;
        p.z = z;
        p.u = u;
        p.h = h;
        p.kind = kind;
        props_.push_back(p);
    };
    prop(4.2f, -0.92f, 36.f, 0);
    prop(7.4f, 0.94f, 32.f, 0);
    prop(11.f, -0.9f, 34.f, 0);
    prop(15.2f, 0.92f, 30.f, 0);
    prop(18.6f, -0.94f, 32.f, 0);
    prop(23.4f, 0.9f, 28.f, 0);
    prop(kBannerZ + 0.35f, -0.62f, 58.f, 1);
    prop(1.4f, -0.72f, 48.f, 1);

    bz_ = kBannerZ;
    bu_ = 0.f;
    carrying_ = false;
    planted_ = false;
}

void Game::bootTitle() {
    layout();
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    face_ = 1;
    lastSec_ = 99;
    fanStep_ = -1;
    watch_ = 0;
    hold_ = 0;
    pz_ = 2.8f;
    u_ = 0;
    vu_ = 0;
    stun_ = 0;
    slip_ = 0;
    shake_ = 0;
    reason_ = "THE WATCH IS OVER";
}

void Game::begin() {
    layout();
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    face_ = 1;
    lastSec_ = 99;
    fanStep_ = -1;
    watch_ = 0;
    hold_ = 0;
    pz_ = 2.8f;
    u_ = 0;
    vu_ = 0;
    stun_ = 0;
    slip_ = 0;
    shake_ = 0;
    t_ = 0;
    reason_ = "THE WATCH IS OVER";
    blip(640.f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.07f);
    beep_ = 0.08f;
}

void Game::fanfare(bool good) {
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = good;
}

void Game::grab() {
    if (carrying_ || stun_ > 0.f) return;
    carrying_ = true;
    planted_ = false;
    shake_ = 0.35f;
    blip(740.f);
    sys_->rumble(0.15f, 0.35f, 70);
}

void Game::plant() {
    if (!carrying_) return;
    carrying_ = false;
    planted_ = true;
    bz_ = 0.f;
    bu_ = 0.f;
    win();
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    reason_ = "THE BANNER IS BACK";
    hold_ = 1.15f;
    shake_ = 0.2f;
    fanfare(true);
    sys_->rumble(0.25f, 0.55f, 160);
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.35f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.24f, 220.f, 0.24f);
    sys_->rumble(0.6f, 0.2f, 200);
}

void Game::tickFoes(float dt) {
    for (Foe& f : foes_) {
        if (!f.on) continue;
        if (f.stun > 0.f) {
            f.stun -= dt;
            continue;
        }
        f.z += float(f.dir) * 0.75f * dt;
        if (f.z > f.hi) {
            f.z = f.hi;
            f.dir = -1;
        } else if (f.z < f.lo) {
            f.z = f.lo;
            f.dir = 1;
        }
    }
}

void Game::bot(float& axisU, float& axisZ, bool& take) const {
    axisU = 0.f;
    axisZ = 0.f;
    take = false;
    if (stun_ > 0.f) return;

    float goalU = 0.f;
    if (!carrying_ && std::fabs(pz_ - bz_) < 3.2f) goalU = bu_;
    float nearest = 99.f;
    for (const Foe& f : foes_) {
        if (!f.on || f.stun > 0.f) continue;
        float gap = std::fabs(f.z - pz_);
        if (gap < 3.1f && gap < nearest) {
            nearest = gap;
            goalU = (u_ >= foeU(f)) ? kDodge : -kDodge;
        }
    }
    axisU = std::clamp((goalU - u_) * 4.2f, -1.f, 1.f);

    if (!carrying_) {
        if (std::fabs(pz_ - bz_) < 1.05f && std::fabs(u_ - bu_) < 0.4f) {
            axisZ = 0.f;
            take = true;
        } else if (bz_ > pz_ + 0.2f) {
            axisZ = 1.f;
        } else if (bz_ < pz_ - 0.2f) {
            axisZ = -1.f;
        }
        return;
    }
    if (pz_ < 1.05f && std::fabs(u_) < 0.48f) {
        axisZ = 0.f;
        take = true;
    } else if (pz_ > 0.45f) {
        axisZ = -1.f;
    }
}

void Game::update() {
    if (stun_ > 0.f) stun_ = std::max(0.f, stun_ - kDt);
    float axisU = 0.f, axisZ = 0.f;
    bool take = false;
    if (bot_) {
        bot(axisU, axisZ, take);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) axisU -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) axisU += 1.f;
        if (std::fabs(pad.axisX) > std::fabs(axisU)) axisU = pad.axisX;
        if (pad.down(gs::BTN_UP)) axisZ += 1.f;
        if (pad.down(gs::BTN_DOWN)) axisZ -= 1.f;
        if (std::fabs(pad.axisY) > std::fabs(axisZ)) axisZ = pad.axisY;
        take = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_Z) || pad.down(gs::BTN_TURBO);
    }
    axisU = std::clamp(axisU, -1.f, 1.f);
    axisZ = std::clamp(axisZ, -1.f, 1.f);

    if (stun_ > 0.f) {
        axisU = 0.f;
        axisZ = 0.f;
        take = false;
    }
    if (axisZ > 0.2f) face_ = 1;
    else if (axisZ < -0.2f) face_ = -1;

    float prevU = u_;
    float wind = windNow();
    if (stun_ <= 0.f) u_ += (axisU * kSide + wind * 0.8f) * kDt;
    else u_ += wind * 0.35f * kDt;
    u_ = std::clamp(u_, -1.35f, 1.35f);
    vu_ = (u_ - prevU) / kDt;

    float spd = axisZ >= 0.f ? kOut : kBack;
    if (stun_ <= 0.f) pz_ += axisZ * spd * kDt;
    pz_ = std::clamp(pz_, 0.f, 34.f);
    if (std::fabs(axisZ) > 0.4f) scroll_ += std::fabs(axisZ) * 150.f * kDt;

    if (std::fabs(u_) > kEdge) slip_ += kDt;
    else slip_ = std::max(0.f, slip_ - kDt * 2.4f);
    if (slip_ > 0.30f) {
        lose("OFF THE RIDGE");
        return;
    }

    tickFoes(kDt);

    for (Foe& f : foes_) {
        if (!f.on || f.stun > 0.f) continue;
        float fu = foeU(f);
        if (std::fabs(f.z - pz_) < 0.7f && std::fabs(fu - u_) < 0.42f) {
            f.stun = 0.75f;
            stun_ = 0.42f;
            shake_ = 0.7f;
            blip(180.f);
            sys_->rumble(0.45f, 0.15f, 120);
            if (carrying_) {
                carrying_ = false;
                bu_ = std::clamp(u_ + (u_ >= fu ? 0.4f : -0.4f), -1.25f, 1.25f);
                bz_ = pz_;
                if (std::fabs(bu_) > 1.08f) {
                    lose("THE BANNER IS GONE");
                    return;
                }
            }
            puffs_.push_back({pz_, u_, 0.35f});
        } else if (!carrying_ && std::fabs(f.z - bz_) < 0.7f && std::fabs(fu - bu_) < 0.28f) {
            f.stun = 0.45f;
            bu_ = std::clamp(bu_ + (fu >= bu_ ? 0.34f : -0.34f), -1.25f, 1.25f);
            bz_ = std::clamp(bz_ + 1.5f, 0.4f, 33.f);
            if (std::fabs(bu_) > 1.08f) {
                lose("THE BANNER IS GONE");
                return;
            }
            blip(220.f);
        }
    }

    if (mode_ != Mode::Play) return;

    bool onBanner = std::fabs(pz_ - bz_) < 0.72f && std::fabs(u_ - bu_) < 0.30f;
    bool nearBanner = std::fabs(pz_ - bz_) < 1.15f && std::fabs(u_ - bu_) < 0.42f;
    if (!carrying_ && stun_ <= 0.f && (onBanner || (take && nearBanner))) grab();

    bool atStaff = pz_ < 0.62f && std::fabs(u_) < 0.40f;
    bool nearStaff = pz_ < 1.05f && std::fabs(u_) < 0.48f;
    if (carrying_ && (atStaff || (take && nearStaff))) {
        plant();
        return;
    }

    watch_ += kDt;
    int sec = int(std::ceil(std::max(0.f, kWatch - watch_)));
    if (sec != lastSec_ && sec <= 5) blip(sec <= 2 ? 880.f : 520.f);
    lastSec_ = sec;
    if (watch_ >= kWatch) lose("THE WATCH IS OVER");

    rush_ = std::fabs(axisZ);
    if ((int(t_ * 9.f) & 1) && rush_ > 0.5f && puffs_.size() < 8) puffs_.push_back({pz_, u_, 0.28f});
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    float wind = std::fabs(windNow());
    if (wind > 0.25f) sys_->apu.noise(0.045f * wind, 900.f + wind * 800.f, false);
    else if (mode_ == Mode::Play && std::fabs(vu_) + std::fabs(pz_) > 0.f) sys_->apu.noise(0.012f, 500.f, true);
    else sys_->apu.noise(0.f, 400.f, false);

    bool moving = mode_ == Mode::Play && (std::fabs(vu_) > 0.4f || stun_ > 0.f);
    if (moving) sys_->apu.tone(2, 62.f, 0.025f);
    else sys_->apu.tone(2, 0.f, 0.f);

    if (fanStep_ < 0) return;
    fanT_ += kDt;
    if (fanT_ < 0.12f) return;
    static const float good[] = {392.f, 523.3f, 659.3f, 784.f};
    static const float bad[] = {196.f, 155.6f, 123.5f};
    const float* notes = fanGood_ ? good : bad;
    int n = fanGood_ ? 4 : 3;
    if (fanStep_ < n) sys_->apu.tone(1, notes[fanStep_], 0.08f);
    else sys_->apu.tone(1, 0.f, 0.f);
    ++fanStep_;
    fanT_ = 0.f;
    if (fanStep_ > n + 3) fanStep_ = -1;
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (!(h > 1.5f) || m.h < 1 || m.w < 1) return;
    h = std::min(h, 420.f);
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::shadow(float cx, float cy, float w) {
    if (w < 6.f) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 6, 420));
    s.h = int16_t(std::max(4, int(std::lround(w * 0.16f))));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -2000, 2000));
    s.img = art_.shadow.pick(float(s.h));
    s.pal = 0;
    s.shadow = true;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 3, 7);
    uint16_t skyHor = mode_ == Mode::Fail ? gs::rgb4(10, 3, 3) : gs::rgb4(13, 7, 4);
    if (mode_ == Mode::Victory) skyHor = gs::rgb4(12, 9, 4);
    v.setFogColor(skyHor);
    int hor = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor) {
            float t = float(y) / float(std::max(hor, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, t * t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor);
        float z = kZScale / std::max(row, 0.5f);
        r.on = true;
        r.cx = 160.f + bendAt(row) + shx_;
        r.hw = halfAt(row);
        r.v = scroll_ + z * 8.f;
        r.pal = uint8_t(PAL_FIELD);
        r.style = gs::ROAD_ROCKY;
        r.band = (int(std::floor(z * 0.55f + pz_ * 0.15f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(11.f - row * 0.09f), 0, 10));
        v.lineBackdrop[y] = mix(gs::rgb4(2, 2, 4), gs::rgb4(0, 0, 1), std::clamp(row / 120.f, 0.f, 1.f));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 71.f) * 4.2f * shake_;
        shy_ = std::cos(t_ * 53.f) * 2.4f * shake_;
    }
    layRoad();

    int flutter = int(t_ * 7.f) & 1;
    bool showCarry = carrying_ && mode_ != Mode::Victory && mode_ != Mode::Fail;
    bool showLoose = !carrying_ && !planted_;
    if (mode_ == Mode::Title) text("S3 RIDGE BANN", 160.f + shx_, 30.f, 0.92f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("IT IS BACK", 160.f + shx_, 32.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail)
        text(reason_, 160.f + shx_, 32.f, std::strlen(reason_) > 16 ? 0.7f : 0.92f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx_, 34.f, 1.05f, PAL_AMBER);

    bool flip = vu_ < -0.15f;
    int fr = (rush_ > 0.35f || std::fabs(vu_) > 0.45f) && (int(t_ * 8.f) & 1) ? 1 : 0;
    if (mode_ == Mode::Title) fr = int(t_ * 6.f) & 1;
    Spot you = spot(u_, kZLine, mode_ == Mode::Victory ? 96.f : 90.f);
    if (you.ok) {
        const gs::Mipped& body = mode_ == Mode::Victory ? art_.plant : art_.runner[fr];
        if (showCarry) {
            float side = flip ? -1.f : 1.f;
            float bob = std::sin(t_ * 9.f) * 2.f;
            spr(art_.banner[flutter], you.x + side * you.h * 0.28f, you.y - you.h * 0.72f + bob, you.h * 0.5f,
                PAL_BANNER, side < 0, 0, false);
        }
        shadow(you.x, you.y, you.h * 0.42f);
        spr(body, you.x, you.y, you.h, PAL_YOU, flip, 0, true);
    }

    if (face_ > 0 && pz_ < 6.f && mode_ != Mode::Title) {
        float near = 1.f - pz_ / 6.f;
        float h = 48.f + near * 64.f;
        float row = kZScale / kZLine;
        float x = 160.f + bendAt(row) + shx_;
        float y = 206.f - near * 8.f;
        spr(art_.staff, x, y, h, PAL_STONE, false, 0, true);
        if (planted_) spr(art_.banner[flutter], x + 8.f, y - h * 0.72f, h * 0.48f, PAL_BANNER, false, 0, false);
    }

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    if (showLoose) {
        float vz = viewZ(bz_);
        if (vz > kZLine - 0.05f) items.push_back({vz, 0, 0});
    }
    if (face_ < 0) {
        float vz = viewZ(0.f);
        if (vz > 0.95f) items.push_back({vz, 1, 0});
    }
    for (int i = 0; i < int(foes_.size()); ++i) {
        if (!foes_[size_t(i)].on) continue;
        float vz = viewZ(foes_[size_t(i)].z);
        if (vz > kZLine - 0.15f) items.push_back({vz, 2, i});
    }
    for (int i = 0; i < int(props_.size()); ++i) {
        float vz = viewZ(props_[size_t(i)].z);
        if (vz > kZLine - 0.15f) items.push_back({vz, 3, i});
    }
    for (int i = 0; i < int(puffs_.size()); ++i) {
        float vz = viewZ(puffs_[size_t(i)].z);
        if (vz > 0.95f) items.push_back({vz, 4, i});
    }
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 0) {
            Spot s = spot(bu_, it.z, 86.f);
            if (!s.ok) continue;
            float bob = std::sin(t_ * 6.f) * 2.f;
            shadow(s.x, s.y, s.h * 0.3f);
            spr(art_.banner[flutter], s.x, s.y - bob, s.h, PAL_BANNER, false, s.fog, true);
            if (s.h > 28.f && (mode_ == Mode::Play || mode_ == Mode::Title))
                text("BANNER", s.x, s.y - s.h - 8.f, 0.42f, PAL_AMBER);
            continue;
        }
        if (it.kind == 1) {
            Spot s = spot(0.f, it.z, 120.f);
            if (!s.ok) continue;
            spr(art_.staff, s.x, s.y, s.h, PAL_STONE, false, s.fog, true);
            if (planted_ || mode_ == Mode::Victory)
                spr(art_.banner[flutter], s.x + s.h * 0.08f, s.y - s.h * 0.62f, s.h * 0.42f, PAL_BANNER, false, s.fog,
                    false);
            if (carrying_ && s.h > 36.f) text("HOME", s.x, s.y - s.h - 6.f, 0.48f, PAL_GOOD);
            continue;
        }
        if (it.kind == 2) {
            const Foe& f = foes_[size_t(it.id)];
            Spot s = spot(foeU(f), it.z, 82.f);
            if (!s.ok) continue;
            int step = int(t_ * 6.f + f.phase) & 1;
            shadow(s.x, s.y, s.h * 0.34f);
            spr(art_.foe[step], s.x, s.y, s.h, PAL_FOE, f.lane > 0, s.fog, true);
            continue;
        }
        if (it.kind == 3) {
            const Prop& p = props_[size_t(it.id)];
            Spot s = spot(p.u, it.z, p.h);
            if (!s.ok) continue;
            spr(p.kind == 1 ? art_.cairn : art_.stone, s.x, s.y, s.h, PAL_STONE, p.u > 0, s.fog, true);
            continue;
        }
        const Puff& p = puffs_[size_t(it.id)];
        Spot s = spot(p.u, it.z, 26.f);
        if (!s.ok) continue;
        float k = std::clamp(p.t / 0.35f, 0.f, 1.f);
        spr(art_.dust, s.x, s.y - (1.f - k) * 8.f, s.h * (0.6f + (1.f - k)), PAL_FX, false, int((1.f - k) * 5.f),
            false);
    }

    int hor = horizon();
    spr(art_.peak[0], 54.f + shx_ * 0.2f, float(hor) + 4.f, 58.f, PAL_MOUNT, false, 2, true);
    spr(art_.peak[1], 262.f + shx_ * 0.2f, float(hor) + 8.f, 46.f, PAL_MOUNT, false, 3, true);
    float drift = std::fmod(t_ * 8.f, 400.f);
    spr(art_.cloud, drift - 50.f, 22.f, 16.f, PAL_FX, false, 3, false);
    spr(art_.cloud, std::fmod(drift + 210.f, 400.f) - 40.f, 34.f, 12.f, PAL_FX, true, 4, false);
    spr(art_.sun, 156.f, 18.f, 22.f, PAL_FX, false, 0, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "BRING THE BANNER BACK", PAL_AMBER);
        hudC(23, "MISS THAT AND THE WATCH IS OVER", PAL_HUD);
        hudC(25, "ARROWS RUN THE SPINE", PAL_HUD);
        hudC(26, "C OR Z TAKES IT AND PLANTS IT", PAL_AMBER);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE BANNER IS BACK ON THE RIDGE", PAL_GOOD);
        hudC(24, "THE WATCH HELD", PAL_AMBER);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(23, "THE WATCH IS OVER", PAL_ALERT);
        hudC(24, reason_, PAL_HUD);
        hudC(26, "START RETRIES", PAL_HUD);
    } else {
        int left = int(std::ceil(std::max(0.f, kWatch - watch_)));
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, buf, left <= 5 ? PAL_ALERT : PAL_HUD);
        if (carrying_) std::snprintf(buf, sizeof buf, "HOME %d", int(std::lround(std::max(0.f, pz_))));
        else std::snprintf(buf, sizeof buf, "CAIRN %d", int(std::lround(std::max(0.f, bz_ - pz_))));
        hud(40 - int(std::strlen(buf)) - 1, 0, buf, carrying_ ? PAL_AMBER : PAL_HUD);
        const char* hint = "RUN TO THE CAIRN";
        int pal = PAL_HUD;
        if (slip_ > 0.02f) {
            hint = "THE DROP";
            pal = PAL_ALERT;
        } else if (std::fabs(windNow()) > 0.4f) {
            hint = "GUST";
            pal = PAL_AMBER;
        } else if (!carrying_ && std::fabs(pz_ - bz_) < 2.4f) {
            hint = "TAKE THE BANNER";
            pal = PAL_AMBER;
        } else if (carrying_ && pz_ < 3.f) {
            hint = "PLANT IT HOME";
            pal = PAL_GOOD;
        } else if (carrying_) {
            hint = "BRING IT BACK";
            pal = PAL_AMBER;
        }
        hudC(1, hint, pal);
        hudC(27, "ARROWS RUN   C OR Z PLANTS", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.22f, 0.12f);
    if (bot_) begin();
    else bootTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        float prev = u_;
        u_ = std::sin(t_ * 0.7f) * 0.28f;
        vu_ = (u_ - prev) / kDt;
        rush_ = 0.6f;
        face_ = 1;
        scroll_ += 16.f * kDt;
        tickFoes(kDt);
        for (Puff& p : puffs_) p.t -= kDt;
        puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }),
                     puffs_.end());
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            vu_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
        for (Puff& p : puffs_) p.t -= kDt;
        puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0.f; }),
                     puffs_.end());
    } else if (mode_ == Mode::Pause) {
        vu_ = 0.f;
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        vu_ *= 0.9f;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }

    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 140, 70);
    else if (mode_ == Mode::Fail) sys.setLight(170, 36, 28);
    else if (mode_ == Mode::Play && carrying_) sys.setLight(170, 110, 36);
    else if (mode_ == Mode::Play && std::fabs(windNow()) > 0.4f) sys.setLight(90, 110, 150);
    else if (mode_ == Mode::Title) sys.setLight(40, 50, 90);
    else sys.setLight(150, 100, 48);
}

}  // namespace rbann

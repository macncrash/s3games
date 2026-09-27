#include "game/purs.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace spanpurs {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kSpanL = 76.f;
constexpr float kSpanR = 244.f;
constexpr float kDeck = 118.f;
constexpr float kSagPx = 26.f;
constexpr float kFallL = 68.f;
constexpr float kFallR = 252.f;
constexpr float kPlayerSpeed = 104.f;
constexpr float kRamReach = 42.f;
constexpr float kRamPush = 26.f;
constexpr float kRivalReach = 16.f;
constexpr float kPi = 3.1415926f;

const char* kKindName[] = {"JACK", "DRUM", "CRAWL", "WAGON", "DRAY"};
const float kDrawH[] = {40.f, 36.f, 32.f, 42.f, 46.f};
// Stack, unflipped, as a fraction of sprite width from centre. + is toward the nose.
const float kStack[] = {-0.28f, 0.18f, -0.22f, 0.02f, -0.08f};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(float(ch(a, s)) + float(ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

void Game::blip(float freq) {
    blip_ = 0.08f;
    blipF_ = freq;
}

int Game::aliveRivals() const {
    int n = 0;
    for (int i = 1; i < kN; ++i)
        if (mach_[size_t(i)].running) ++n;
    return n;
}

int Game::quarry() const {
    if (focus_ > 0 && focus_ < kN && mach_[size_t(focus_)].running) return focus_;
    int best = -1;
    float bd = 1e9f;
    for (int i = 1; i < kN; ++i) {
        if (!mach_[size_t(i)].running) continue;
        float d = std::fabs(mach_[size_t(i)].x - px_);
        if (best < 0 || d < bd - 0.05f || (std::fabs(d - bd) <= 0.05f && i < best)) {
            bd = d;
            best = i;
        }
    }
    return best;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 3;
    if (bannerT_ > 0.05f) return 2;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

int Game::bayAt(float x) const {
    if (x < kSpanL || x > kSpanR) return -1;
    float w = (kSpanR - kSpanL) / float(kBays);
    int b = int((x - kSpanL) / w);
    if (b < 0) return 0;
    if (b >= kBays) return kBays - 1;
    return b;
}

float Game::bayCenter(int i) const {
    float w = (kSpanR - kSpanL) / float(kBays);
    return kSpanL + (float(i) + 0.5f) * w;
}

float Game::sagAt(float x) const {
    float w = (kSpanR - kSpanL) / float(kBays);
    float u = (x - kSpanL) / w - 0.5f;
    if (u <= 0.f) return bay_[0].sag;
    if (u >= float(kBays - 1)) return bay_[kBays - 1].sag;
    int i = int(u);
    float f = u - float(i);
    return bay_[size_t(i)].sag * (1.f - f) + bay_[size_t(i + 1)].sag * f;
}

float Game::footY(float x) const { return kDeck + sagAt(x) * kSagPx; }

float Game::cableY(float u) const {
    float avg = 0.f;
    for (const Bay& b : bay_) avg += b.sag;
    avg /= float(kBays);
    float bob = std::sin(t_ * 1.5f + u * 5.f) * (mode_ == Mode::Title ? 1.3f : 0.6f);
    return 36.f + std::sin(u * kPi) * (15.f + avg * 12.f) + bob;
}

const gs::Mipped& Game::body(int kind, int fr) const {
    fr &= 1;
    switch (kind) {
        case 1: return art_.drum[fr];
        case 2: return art_.crawl[fr];
        case 3: return art_.wagon[fr];
        case 4: return art_.dray[fr];
        default: return art_.jack[fr];
    }
}

int Game::palFor(const Mach& m) const {
    if (m.flash > 0.f) return PAL_FX;
    if (!m.running) return PAL_DEAD;
    return PAL_JACK + m.kind;
}

void Game::boot() {
    t_ = 0;
    begin();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    banner_.clear();
    bannerT_ = 0;
}

void Game::begin() {
    struct Seed {
        int kind;
        float x;
        int hp;
        float speed;
        float weight;
    };
    const Seed seed[kN] = {
        {0, 170.f, 8, kPlayerSpeed, 1.0f},
        {1, 104.f, 4, 46.f, 1.35f},
        {2, 142.f, 3, 60.f, 1.05f},
        {3, 200.f, 3, 70.f, 1.15f},
        {4, 230.f, 4, 42.f, 1.30f},
    };
    for (int i = 0; i < kN; ++i) {
        Mach& m = mach_[size_t(i)];
        m = Mach{};
        m.kind = seed[i].kind;
        m.x = seed[i].x;
        m.home = seed[i].x;
        m.hp = seed[i].hp;
        m.maxHp = seed[i].hp;
        m.speed = seed[i].speed;
        m.weight = seed[i].weight;
        m.face = (i == 0) ? -1 : (seed[i].x < 170.f ? 1 : -1);
        m.running = true;
    }
    px_ = mach_[0].x;
    face_ = -1;
    for (int i = 0; i < kBays; ++i) {
        bay_[size_t(i)].sag = 0.16f + 0.05f * std::sin(float(i) * 1.2f);
        bay_[size_t(i)].load = 0.2f;
    }
    motes_.clear();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    reason_ = "NOT THE LAST";
    banner_.clear();
    stalled_ = 0;
    score_ = 0;
    focus_ = -1;
    cool_ = 0;
    grace_ = 0.45f;
    shake_ = 0;
    bannerT_ = 0;
    blip_ = 0;
    playT_ = 0;
    run_ = 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(8, 5, 4));
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        sys.vdp.road[y] = {};
        sys.vdp.lineFog[y] = 0;
    }
    boot();
    draw();
}

void Game::puff(float x, float y, int n) {
    for (int i = 0; i < n; ++i) {
        Mote m;
        m.x = x;
        m.y = y;
        m.a = 0;
        m.vx = (float(i) - float(n) * 0.5f) * 22.f;
        m.vy = -28.f - float(i) * 6.f;
        motes_.push_back(m);
    }
    if (motes_.size() > 36) motes_.erase(motes_.begin(), motes_.begin() + int(motes_.size() - 36));
}

void Game::tickMotes() {
    for (Mote& m : motes_) {
        m.a += kDt;
        m.x += m.vx * kDt;
        m.y += m.vy * kDt;
        m.vy += 40.f * kDt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.a > 0.55f; }), motes_.end());
}

void Game::stall(int i, bool fall) {
    Mach& m = mach_[size_t(i)];
    if (!m.running || mode_ != Mode::Play) return;
    m.running = false;
    m.hp = 0;
    m.vx = 0;
    m.fell = fall || m.x < kFallL || m.x > kFallR;
    if (i == 0) {
        loseSpan(fall || m.fell ? "THE SPAN TOOK YOU" : "THE BOILER SEIZED");
        return;
    }
    ++stalled_;
    score_ += 100;
    banner_ = std::string("THE ") + kKindName[m.kind] + " SEIZED";
    bannerT_ = 1.2f;
    shake_ = std::max(shake_, 3.2f);
    if (focus_ == i) focus_ = -1;
    sys_->apu.noiseBurst(0.32f, fall ? 140.f : 220.f, 0.16f);
    blip(150.f);
    puff(m.x, footY(m.x) - 10.f, 5);
    if (aliveRivals() == 0 && mach_[0].running) winSpan();
}

void Game::winSpan() {
    if (mode_ != Mode::Play || !mach_[0].running) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "THE LAST MACHINE STILL RUNNING";
    score_ += boiler() * 25;
    banner_ = "THE LAST MACHINE STILL RUNNING";
    bannerT_ = 2.f;
}

void Game::loseSpan(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    reason_ = why;
    mach_[0].running = false;
    mach_[0].hp = 0;
}

void Game::hurtPlayer(float push) {
    Mach& p = mach_[0];
    if (!p.running || mode_ != Mode::Play) return;
    p.hp -= 1;
    p.flash = 0.16f;
    p.x += push;
    px_ = p.x;
    shake_ = 3.4f;
    score_ = std::max(0, score_ - 10);
    sys_->rumble(0.45f, 0.75f, 80);
    sys_->apu.noiseBurst(0.28f, 260.f, 0.1f);
    blip(190.f);
    puff(p.x, footY(p.x) - 14.f, 3);
    if (p.hp <= 0) stall(0, false);
}

void Game::doRam() {
    if (cool_ > 0.f || !mach_[0].running || mode_ != Mode::Play) return;
    int q = quarry();
    int hit = -1;
    auto front = [&](const Mach& m) {
        float dx = m.x - px_;
        if (face_ >= 0) return dx > -8.f && dx < kRamReach;
        return dx < 8.f && dx > -kRamReach;
    };
    if (q > 0 && mach_[size_t(q)].running && front(mach_[size_t(q)])) {
        hit = q;
    } else {
        float best = kRamReach;
        for (int i = 1; i < kN; ++i) {
            const Mach& m = mach_[size_t(i)];
            if (!m.running || !front(m)) continue;
            float adx = std::fabs(m.x - px_);
            if (adx < best) {
                best = adx;
                hit = i;
            }
        }
    }
    if (hit < 0) {
        px_ += float(face_) * 8.f;
        mach_[0].x = px_;
        cool_ = 0.2f;
        blip(300.f);
        return;
    }
    Mach& m = mach_[size_t(hit)];
    m.hp -= 1;
    m.flash = 0.14f;
    m.stun = 0.32f;
    m.press = 0.f;
    m.cool = std::max(m.cool, 0.2f);
    m.x += float(face_) * kRamPush;
    cool_ = 0.44f;
    shake_ = 2.2f;
    score_ += 5;
    sys_->rumble(0.22f, 0.4f, 50);
    sys_->apu.noiseBurst(0.2f, 680.f, 0.07f);
    blip(660.f);
    puff((px_ + m.x) * 0.5f, footY(m.x) - 12.f, 4);
    if (m.hp <= 0) stall(hit, false);
}

void Game::driveRivals() {
    int q = focus_;
    for (int i = 1; i < kN; ++i) {
        Mach& m = mach_[size_t(i)];
        if (!m.running) {
            m.vx = 0;
            continue;
        }
        if (m.stun > 0.f) {
            m.vx = 0;
            m.press = 0;
            continue;
        }
        float goal = m.home + std::sin(t_ * 0.75f + float(i) * 1.7f) * 7.f;
        float spd = m.speed * 0.32f;
        if (i == q) {
            goal = px_;
            spd = m.speed;
        }
        float dx = goal - m.x;
        float dir = 0.f;
        if (dx > 2.5f) dir = 1.f;
        else if (dx < -2.5f) dir = -1.f;
        if (i == q && std::fabs(dx) < 12.f) dir = 0.f;
        if (dir != 0.f) m.face = int(dir);
        m.vx = dir * spd;
        m.x += m.vx * kDt;
        m.x = std::clamp(m.x, 86.f, 234.f);
    }
}

void Game::rivalRams() {
    if (grace_ > 0.f || mode_ != Mode::Play) return;
    for (int i = 1; i < kN; ++i) {
        Mach& m = mach_[size_t(i)];
        if (!m.running || m.stun > 0.f) {
            m.press = 0;
            continue;
        }
        float adx = std::fabs(px_ - m.x);
        if (adx > kRivalReach) {
            m.press = 0;
            continue;
        }
        m.press += kDt;
        if (m.press < 0.32f || m.cool > 0.f) continue;
        float push = (px_ >= m.x ? 1.f : -1.f) * 14.f;
        m.press = 0;
        m.cool = 0.9f;
        m.stun = 0.1f;
        hurtPlayer(push);
        if (mode_ != Mode::Play) return;
    }
}

void Game::separate() {
    for (int i = 0; i < kN; ++i) {
        for (int j = i + 1; j < kN; ++j) {
            Mach& a = mach_[size_t(i)];
            Mach& b = mach_[size_t(j)];
            if (!a.running || !b.running) continue;
            float dx = b.x - a.x;
            float adx = std::fabs(dx);
            if (adx >= 13.f || adx < 0.01f) continue;
            float push = (13.f - adx) * 0.5f;
            float s = dx >= 0.f ? 1.f : -1.f;
            if (a.stun > 0.f && b.stun <= 0.f) b.x += s * push * 2.f;
            else if (b.stun > 0.f && a.stun <= 0.f) a.x -= s * push * 2.f;
            else {
                a.x -= s * push;
                b.x += s * push;
            }
        }
    }
    px_ = mach_[0].x;
}

void Game::edges() {
    for (int i = 0; i < kN; ++i) {
        Mach& m = mach_[size_t(i)];
        if (!m.running) continue;
        if (m.x < kFallL || m.x > kFallR) stall(i, true);
        if (mode_ != Mode::Play) return;
    }
}

void Game::springs() {
    float load[kBays] = {};
    for (const Mach& m : mach_) {
        if (m.fell) continue;
        int b = bayAt(m.x);
        if (b >= 0) load[b] += m.weight * (m.running ? 0.18f : 0.1f);
    }
    for (int i = 0; i < kBays; ++i) {
        float target = std::min(0.72f, load[i]);
        bay_[size_t(i)].sag += (target - bay_[size_t(i)].sag) * std::min(1.f, 3.2f * kDt);
        bay_[size_t(i)].load = load[i];
    }
}

void Game::steam() {
    if (mode_ != Mode::Play) return;
    auto spit = [&](int i) {
        if (i < 0 || !mach_[size_t(i)].running) return;
        float phase = playT_ * 2.6f + float(i) * 0.37f;
        float prev = phase - kDt * 2.6f;
        if (int(phase) == int(prev)) return;
        float y = footY(mach_[size_t(i)].x) - kDrawH[mach_[size_t(i)].kind] * 0.85f;
        puff(mach_[size_t(i)].x, y, 1);
    };
    spit(0);
    spit(quarry());
}

void Game::botIntent(float& dir, bool& ram, bool& go) {
    dir = 0;
    ram = false;
    go = false;
    if (mode_ != Mode::Play) {
        if (mode_ == Mode::Title && t_ > 0.45f) go = true;
        return;
    }
    int q = quarry();
    if (q < 0) return;
    const Mach& m = mach_[size_t(q)];
    float dx = m.x - px_;
    float adx = std::fabs(dx);
    float want = dx >= 0.f ? 1.f : -1.f;
    if (cool_ > 0.05f) {
        float escape = -want;
        bool blocked = (escape < 0.f && px_ < 112.f) || (escape > 0.f && px_ > 208.f);
        if (blocked) dir = want;
        else if (adx < 46.f) dir = escape;
        else if (adx > 68.f) dir = want;
    } else {
        if (adx > 20.f) dir = want;
        if (adx < 36.f && adx > 4.f) ram = true;
    }
}

void Game::readPad(float& dir, bool& ram, bool& go) {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.25f) dir = std::clamp(p.axisX, -1.f, 1.f);
    ram = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_TURBO);
    bool tap = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO) || p.pressed(gs::BTN_START);
    bool start = p.pressed(gs::BTN_START);
    if (mode_ == Mode::Play || mode_ == Mode::Pause) go = start;
    else go = start || tap;
}

void Game::step(float dir, bool ram) {
    playT_ += kDt;
    if (grace_ > 0.f) grace_ -= kDt;
    if (bannerT_ > 0.f) bannerT_ -= kDt;
    shake_ = std::max(0.f, shake_ - kDt * 8.f);
    cool_ = std::max(0.f, cool_ - kDt);
    for (Mach& m : mach_) {
        m.cool = std::max(0.f, m.cool - kDt);
        m.flash = std::max(0.f, m.flash - kDt);
        if (m.stun > 0.f) m.stun -= kDt;
        if (m.fell) {
            m.drop += 130.f * kDt;
            if (!m.splashed && footY(m.x) + m.drop > 188.f) {
                m.splashed = true;
                sys_->apu.noiseBurst(0.26f, 360.f, 0.18f);
            }
        }
    }

    int q = quarry();
    if (q != focus_) {
        focus_ = q;
        if (q > 0 && playT_ > 0.15f) blip(780.f);
    }

    if (dir > 0.2f) face_ = 1;
    else if (dir < -0.2f) face_ = -1;
    if (bot_) {
        if (px_ < 96.f && dir < 0.f) dir = 0.f;
        if (px_ > 224.f && dir > 0.f) dir = 0.f;
    }
    run_ = std::fabs(dir) * kPlayerSpeed;
    px_ += dir * kPlayerSpeed * kDt;
    mach_[0].x = px_;
    mach_[0].face = face_;

    driveRivals();
    if (ram) doRam();
    if (mode_ != Mode::Play) {
        springs();
        tickMotes();
        return;
    }
    rivalRams();
    if (mode_ != Mode::Play) {
        springs();
        tickMotes();
        return;
    }
    separate();
    px_ = mach_[0].x;
    if (bot_) px_ = std::clamp(px_, 96.f, 224.f);
    mach_[0].x = px_;
    edges();
    springs();
    steam();
    tickMotes();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += kDt;

    float dir = 0;
    bool ram = false;
    bool go = false;
    if (bot_) botIntent(dir, ram, go);
    else readPad(dir, ram, go);

    if (go) {
        if (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Fail) begin();
        else if (mode_ == Mode::Play) mode_ = Mode::Pause;
        else if (mode_ == Mode::Pause) mode_ = Mode::Play;
    }

    if (mode_ == Mode::Play) step(dir, ram);
    else if (mode_ == Mode::Victory || mode_ == Mode::Fail) {
        if (bannerT_ > 0.f) bannerT_ -= kDt;
        shake_ = std::max(0.f, shake_ - kDt * 6.f);
        for (Mach& m : mach_) {
            if (!m.fell) continue;
            m.drop += 110.f * kDt;
        }
        tickMotes();
    }

    draw();
    serviceAudio();

    if (mode_ == Mode::Victory) sys.setLight(40, 180, 70);
    else if (mode_ == Mode::Fail) sys.setLight(180, 30, 24);
    else if (cool_ > 0.2f) sys.setLight(180, 130, 40);
    else sys.setLight(36, 42, 64);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 24 || s.y + s.h < -24) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();

    const uint16_t sky0 = gs::rgb4(5, 3, 7);
    const uint16_t sky1 = gs::rgb4(14, 8, 6);
    const uint16_t hor = gs::rgb4(11, 9, 7);
    const uint16_t gorge0 = gs::rgb4(3, 4, 4);
    const uint16_t gorge1 = gs::rgb4(2, 3, 3);
    const uint16_t water0 = gs::rgb4(2, 5, 6);
    const uint16_t water1 = gs::rgb4(1, 2, 3);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        uint16_t c;
        if (y < 78) c = lerpC(sky0, sky1, y / 78.f);
        else if (y < 108) c = lerpC(sky1, hor, (y - 78) / 30.f);
        else if (y < 168) c = lerpC(gorge0, gorge1, (y - 108) / 60.f);
        else c = lerpC(water0, water1, (y - 168) / 56.f);
        vdp.lineBackdrop[y] = c;
    }

    float jig = shake_;
    float ox = std::sin(t_ * 90.f) * jig;
    float oy = std::cos(t_ * 70.f) * jig * 0.3f;

    if (mode_ == Mode::Title) spr(art_.title, 160, 18, 26, PAL_TITLE, false, 0, false);
    else if (mode_ == Mode::Victory) spr(art_.last, 160, 20, 28, PAL_GOOD, false, 0, false);
    else if (mode_ == Mode::Fail) spr(art_.seized, 160, 20, 26, PAL_ALERT, false, 0, false);
    else if (mode_ == Mode::Pause) spr(art_.paused, 160, 20, 26, PAL_AMBER, false, 0, false);

    int q = quarry();
    if ((mode_ == Mode::Play || mode_ == Mode::Pause) && q > 0) {
        float bob = std::sin(t_ * 8.f) * 2.f;
        spr(art_.chev, mach_[size_t(q)].x + ox, footY(mach_[size_t(q)].x) - kDrawH[mach_[size_t(q)].kind] - 8.f + bob + oy,
            12.f, PAL_ALERT, false, 0, false);
    }
    if (mach_[0].running || mode_ == Mode::Title) {
        float bob = std::sin(t_ * 6.f) * 1.5f;
        spr(art_.chev, px_ + ox, footY(px_) - kDrawH[0] - 6.f + bob + oy, 9.f, PAL_GOOD, false, 0, false);
    }

    int ff = int(t_ * 10.f) & 1;
    for (int i = 0; i < kN; ++i) {
        const Mach& m = mach_[size_t(i)];
        if (m.fell && footY(m.x) + m.drop > gs::SCREEN_H + 20.f) continue;
        bool live = m.running || mode_ == Mode::Title;
        int fr = live ? (int(t_ * 7.f + m.x * 0.08f) & 1) : 0;
        float h = kDrawH[m.kind];
        float y = footY(m.x) + (m.fell ? m.drop : 0.f) + (live && fr ? 0.f : 1.f);
        float x = m.x + ox + (m.fell ? std::sin(m.drop * 0.12f) * 4.f : 0.f);
        bool flip = m.face < 0;
        if (live) {
            float sw = h * float(body(m.kind, fr).w) / float(body(m.kind, fr).h);
            float su = kStack[m.kind];
            if (flip) su = -su;
            spr(art_.flame[ff], x + su * sw, y - h * 0.92f, 12.f, PAL_FX, false, 0, false);
        } else if (!m.fell) {
            spr(art_.dust, x, y - h * 0.7f + std::sin(t_ * 3.f + float(i)) * 2.f, 8.f, PAL_FX, false, 2, false);
        }
        spr(body(m.kind, fr), x, y + oy, h, palFor(m), flip, 0, true);
    }

    for (const Mote& m : motes_) spr(art_.dust, m.x + ox, m.y + oy, 7.f + (0.4f - m.a) * 8.f, PAL_FX, false, 0, false);

    for (int i = 0; i < kBays; ++i) {
        float cx = bayCenter(i) + ox;
        float surface = kDeck + bay_[size_t(i)].sag * kSagPx + oy;
        spr(art_.plank, cx, surface + 5.f, 13.f, PAL_IRON, false, 0, false);
    }
    for (int i = 0; i < kBays; ++i) {
        float cx = bayCenter(i) + ox;
        float surface = kDeck + bay_[size_t(i)].sag * kSagPx + oy;
        spr(art_.truss, cx, surface + 18.f, 16.f, PAL_IRON, false, 1, false);
    }
    spr(art_.lip, 70 + ox, kDeck + 8.f + oy, 16.f, PAL_ROCK, false, 0, false);
    spr(art_.lip, 250 + ox, kDeck + 8.f + oy, 16.f, PAL_ROCK, true, 0, false);

    for (int i = 0; i < kBays; ++i) {
        float u = (bayCenter(i) - kSpanL) / (kSpanR - kSpanL);
        float y0 = cableY(u) + oy;
        float y1 = kDeck + bay_[size_t(i)].sag * kSagPx + oy;
        float h = std::max(8.f, y1 - y0);
        spr(art_.rope, bayCenter(i) + ox, (y0 + y1) * 0.5f, h, PAL_ROPE, false, 0, false);
    }
    for (int i = 0; i <= 16; ++i) {
        float u = float(i) / 16.f;
        float x = kSpanL + (kSpanR - kSpanL) * u;
        spr(art_.link, x + ox, cableY(u) + oy, 7.f, PAL_ROPE, false, 0, false);
    }

    spr(art_.tower, 58 + ox, 168 + oy, 138.f, PAL_IRON, false, 0, true);
    spr(art_.tower, 262 + ox, 168 + oy, 138.f, PAL_IRON, true, 0, true);
    spr(art_.pennant, 78 + ox, 48 + oy, 18.f, PAL_JACK, false, 0, true);
    spr(art_.flame[ff], 250 + ox, 42 + oy, 12.f, PAL_FX, false, 0, false);

    for (int i = 0; i < 5; ++i) {
        float x = 108.f + float(i) * 22.f + std::sin(t_ * 1.6f + float(i)) * 4.f;
        float y = 196.f + float(i % 2) * 6.f + std::sin(t_ * 2.2f + float(i)) * 2.f;
        spr(art_.dust, x, y, 4.f, PAL_MIST, false, 1, false);
    }

    spr(art_.cliff, 24 + ox, 226 + oy, 156.f, PAL_ROCK, false, 0, true);
    spr(art_.cliff, 296 + ox, 226 + oy, 156.f, PAL_ROCK, true, 0, true);
    spr(art_.bird, std::fmod(t_ * 18.f, 400.f) - 30.f, 42.f + std::sin(t_ * 2.f) * 3.f, 8.f, PAL_MIST, false, 0, false);
    spr(art_.bird, std::fmod(t_ * 12.f + 160.f, 420.f) - 40.f, 30.f, 6.f, PAL_MIST, true, 0, false);
    spr(art_.cloud, std::fmod(20.f + t_ * 7.f, 460.f) - 70.f, 26.f, 20.f, PAL_MIST, false, 2, false);
    spr(art_.cloud, std::fmod(180.f + t_ * 4.f, 500.f) - 80.f, 16.f, 24.f, PAL_MIST, false, 3, false);
    spr(art_.sun, 286, 30, 22.f, PAL_MIST, false, 0, false);

    if (mode_ == Mode::Title) {
        hudC(16, "YOU HAVE THE SPAN", PAL_AMBER);
        hudC(17, "BE THE LAST MACHINE STILL RUNNING", PAL_TITLE);
        hudC(18, "ANYTHING ELSE IS A LOSS", PAL_HUD);
        hudC(20, "ARROWS DRIVE ALONG THE SPAN", PAL_HUD);
        hudC(21, "Z OR SPACE RAMS A MACHINE", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(23, "START TO RUN", PAL_GOOD);
        hud(1, 27, S3_VERSION_STRING, PAL_HUD);
        hud(34, 27, "S3-16", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hudC(16, "THE LAST MACHINE STILL RUNNING", PAL_GOOD);
        hudC(17, "THE SPAN IS YOURS", PAL_AMBER);
        hudC(23, "START TO RUN IT AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(16, reason_, PAL_ALERT);
        hudC(17, "NOT THE LAST MACHINE", PAL_HUD);
        hudC(23, "START TO TRY AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(23, "START TO GO ON", PAL_HUD);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 SPAN PURSUIT", PAL_TITLE);
        int live = aliveRivals() + (mach_[0].running ? 1 : 0);
        std::string liveS = "LIVE " + std::to_string(live);
        hud(40 - int(liveS.size()), 0, liveS, live == 1 ? PAL_GOOD : PAL_HUD);

        std::string row;
        for (int i = 1; i < kN; ++i) {
            if (i > 1) row += "  ";
            row += kKindName[mach_[size_t(i)].kind];
            row += " ";
            row += std::to_string(std::max(0, mach_[size_t(i)].hp));
        }
        hud(1, 24, row, PAL_HUD);

        bool lip = px_ < 96.f || px_ > 224.f;
        if (lip && mode_ == Mode::Play) hudC(25, "THE LIP", PAL_ALERT);
        else if (bannerT_ > 0.f && mode_ == Mode::Play) hudC(25, banner_, PAL_ALERT);
        else if (aliveRivals() == 1) hudC(25, "ONE MACHINE LEFT", PAL_AMBER);
        else if (q > 0) {
            std::string s = std::string("THE ") + kKindName[mach_[size_t(q)].kind] + " IS CLOSING";
            hudC(25, s, PAL_AMBER);
        }

        std::string boiler = "BOILER ";
        for (int i = 0; i < mach_[0].maxHp; ++i) boiler += (i < mach_[0].hp && mach_[0].running) ? '#' : '.';
        int bpal = mach_[0].hp <= 2 ? PAL_ALERT : PAL_GOOD;
        hud(1, 26, boiler, bpal);
        hud(1, 27, "ARROWS DRIVE    Z OR SPACE RAMS", PAL_HUD);
    }
}

void Game::serviceAudio() {
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Victory) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f, 880.f};
        int step = int(t_ * 7.f) % 6;
        a.tone(0, notes[step], 0.055f);
        a.tone(1, notes[(step + 2) % 6] * 0.5f, 0.028f);
        a.tone(2, 0, 0);
        return;
    }
    if (mode_ == Mode::Fail) {
        a.tone(0, 82.f, 0.04f);
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
        return;
    }
    if (blip_ > 0.f) {
        blip_ -= kDt;
        a.tone(2, blipF_, 0.06f);
    } else {
        a.tone(2, 0, 0);
    }
    if (mode_ == Mode::Play && mach_[0].running) {
        float hum = 52.f + run_ * 0.22f;
        a.tone(0, hum, 0.03f);
        a.tone(1, hum * 1.5f, 0.012f);
    } else if (mode_ == Mode::Title) {
        a.tone(0, 98.f, 0.022f);
        a.tone(1, 147.f, 0.012f);
    } else {
        a.tone(0, 0, 0);
        a.tone(1, 0, 0);
    }
}

}  // namespace spanpurs

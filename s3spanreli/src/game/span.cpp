#include "game/span.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace spanreli {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kBays = 5;
constexpr int kColumn = 6;
constexpr float kSpanL = 70.f;
constexpr float kSpanR = 246.f;
constexpr float kDeck = 112.f;
constexpr float kSagPx = 36.f;
constexpr float kBreak = 0.80f;
constexpr float kRun = 220.f;
constexpr float kWalk = 46.f;
constexpr float kSpring = 3.0f;
constexpr float kHold = 3.4f;
constexpr float kGust = 0.72f;
constexpr float kBase = 0.05f;
constexpr float kWeight = 0.15f;
constexpr float kGapT = 1.80f;
constexpr float kWarnT = 1.45f;
constexpr float kBlowT = 1.10f;
constexpr float kBellAt = 12.0f;
constexpr float kBellEnd = 22.5f;
constexpr float kRope = 0.80f;
// The bell hangs on the far pier, past the last bay. Hauling does not brace.
constexpr float kBellX = 268.f;
constexpr float kStand = 284.f;
constexpr float kAnswer = 270.f;
constexpr float kPi = 3.14159265f;

const int kGustOrder[10] = {2, 0, 4, 1, 3, 4, 0, 2, 3, 1};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

char strainMark(float s) {
    if (s >= 0.72f) return '#';
    if (s >= 0.55f) return '!';
    if (s >= 0.38f) return '+';
    if (s >= 0.20f) return ':';
    return '.';
}

gs::FMPatch bellVoice() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.04f;
    p.op[0] = {1.00f, 0.90f, 0.003f, 0.48f, 0.0f, 0.90f};
    p.op[1] = {2.71f, 0.32f, 0.004f, 0.36f, 0.0f, 0.70f};
    p.op[2] = {5.43f, 0.14f, 0.005f, 0.24f, 0.0f, 0.55f};
    p.op[3] = {8.07f, 0.07f, 0.006f, 0.16f, 0.0f, 0.40f};
    p.vol = 0.18f;
    p.echo = 0.30f;
    return p;
}

}  // namespace

int Game::score() const {
    int s = held_ * 50 + int(std::lround(rope_ * 80.f));
    if (won_) s += 300;
    return s;
}

int Game::marker() const {
    if (over_ || mode_ == Mode::Victory || mode_ == Mode::Over) return 3;
    if ((mode_ == Mode::Watch || mode_ == Mode::Pause) && bell_) return 2;
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::reset() {
    over_ = won_ = bell_ = bracing_ = hauling_ = faceLeft_ = inZone_ = peal_ = false;
    reason_ = "THE WATCH RAN OUT";
    held_ = 0;
    gustBay_ = -1;
    gustN_ = 0;
    fanStep_ = -1;
    watch_ = rope_ = phaseT_ = shake_ = endT_ = bellTick_ = marchT_ = tick_ = blip_ = 0;
    blipF_ = 440.f;
    phase_ = Phase::Gap;
    motes_.clear();
    for (Bay& b : bay_) b = {};
}

void Game::place(bool posed) {
    const Kind kinds[kColumn] = {Kind::Drum, Kind::Coat, Kind::Coat, Kind::Coat, Kind::Coat, Kind::Wagon};
    const float station[kColumn] = {96.f, 132.f, 160.f, 196.f, 208.f, 232.f};
    const float load[kColumn] = {1.00f, 1.05f, 1.10f, 0.95f, 0.90f, 1.60f};
    for (int i = 0; i < kColumn; i++) {
        Member& m = col_[size_t(i)];
        m.kind = kinds[i];
        m.station = station[i];
        m.load = load[i];
        m.x = posed ? station[i] : station[i] - 72.f;
        m.drop = 0;
        m.fell = false;
    }
    px_ = 158.f;
    faceLeft_ = false;
}

void Game::boot() {
    reset();
    place(true);
    mode_ = Mode::Title;
}

void Game::begin() {
    reset();
    place(false);
    mode_ = Mode::Watch;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 3, 6));
    sys.apu.setPatch(0, bellVoice());
    sys.apu.setPatch(1, bellVoice());
    t_ = 0;
    boot();
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

float Game::footY(float x) const {
    int b = bayAt(x);
    if (b < 0) return kDeck;
    return kDeck + bay_[size_t(b)].sag * kSagPx;
}

float Game::cableY(float u) const {
    float avg = 0;
    for (const Bay& b : bay_) avg += b.sag;
    avg /= float(kBays);
    float droop = std::sin(u * kPi) * (8.f + avg * 16.f);
    float bob = std::sin(t_ * 1.6f + u * 4.f) * (mode_ == Mode::Title ? 1.4f : 0.6f);
    return 46.f + droop + bob;
}

int Game::heaviest() const {
    int heavy = -1;
    float best = 0.2f;
    for (int i = 0; i < kBays; i++) {
        float L = bay_[size_t(i)].load;
        if (L > best + 0.001f || (heavy >= 0 && std::fabs(L - best) <= 0.001f && i > heavy)) {
            best = L;
            heavy = i;
        }
    }
    return heavy;
}

int Game::pickGust() const {
    int want = kGustOrder[gustN_ % 10];
    if (bay_[size_t(want)].load > 0.2f) return want;
    return heaviest();
}

void Game::botIntent(float& dir, bool& brace, bool& haul, bool& go) {
    dir = 0;
    brace = false;
    haul = false;
    go = false;
    if (mode_ == Mode::Title) {
        if (t_ > 0.35f) go = true;
        return;
    }
    if (mode_ != Mode::Watch) return;

    int threat = -1;
    if (phase_ != Phase::Gap && gustBay_ >= 0) threat = gustBay_;
    for (int i = 0; i < kBays; i++) {
        if (bay_[size_t(i)].sag > 0.52f && (threat < 0 || bay_[size_t(i)].sag > bay_[size_t(threat)].sag)) threat = i;
    }
    bool gusting = phase_ != Phase::Gap && gustBay_ >= 0;
    bool hot = false;
    for (const Bay& b : bay_)
        if (b.sag > 0.52f) hot = true;
    bool danger = gusting || hot;

    if (bell_ && rope_ < kRope && !danger) {
        if (px_ < kStand - 6.f) dir = 1.f;
        else haul = true;
        return;
    }

    int goal = threat;
    if (goal < 0) goal = heaviest();
    if (goal < 0) goal = 2;
    float tx = bayCenter(goal);
    if (px_ < tx - 6.f) dir = 1.f;
    else if (px_ > tx + 6.f) dir = -1.f;
    int here = bayAt(px_);
    if (here >= 0 && (here == goal || bay_[size_t(here)].sag > 0.40f || (gusting && here == gustBay_))) brace = true;
}

void Game::readPad(float& dir, bool& brace, bool& haul, bool& go) {
    const gs::Pad& p = sys_->pad;
    dir = 0;
    if (p.down(gs::BTN_LEFT)) dir -= 1.f;
    if (p.down(gs::BTN_RIGHT)) dir += 1.f;
    if (dir == 0.f && std::fabs(p.axisX) > 0.25f) dir = std::clamp(p.axisX, -1.f, 1.f);
    brace = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_Z) ||
            p.down(gs::BTN_TURBO) || p.down(gs::BTN_DOWN);
    haul = p.down(gs::BTN_UP) || p.axisY > 0.45f;
    bool start = p.pressed(gs::BTN_START);
    bool tap = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO) || p.pressed(gs::BTN_Z);
    if (mode_ == Mode::Watch || mode_ == Mode::Pause) go = start;
    else go = start || tap;
}

void Game::winWatch() {
    if (over_) return;
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    reason_ = "THE SPAN HELD UNTIL THE RELIEF BELL";
    shake_ = 0;
    if (!bot_) sys_->rumble(0.22f, 0.1f, 140);
}

void Game::loseWatch(const char* why) {
    if (over_) return;
    mode_ = Mode::Over;
    over_ = true;
    won_ = false;
    reason_ = why;
    shake_ = 3.4f;
    sys_->apu.noiseBurst(0.48f, 150.f, 0.4f);
    sys_->apu.tone(1, 55.f, 0.1f);
    if (!bot_) sys_->rumble(0.7f, 0.3f, 180);
    std::fprintf(stderr, "spanreli %s t=%.2f rope %.2f px %.0f\n", why, watch_, rope_, px_);
}

void Game::stepWatch(float dir, bool brace, bool haul) {
    watch_ += kDt;
    if (dir < -0.2f) faceLeft_ = true;
    else if (dir > 0.2f) faceLeft_ = false;
    px_ = std::clamp(px_ + dir * kRun * kDt, 26.f, 306.f);

    bool marching = false;
    for (Member& m : col_) {
        if (m.fell) continue;
        if (m.x < m.station) {
            m.x = std::min(m.station, m.x + kWalk * kDt);
            marching = true;
        }
    }
    if (marching) {
        marchT_ += kDt;
        if (marchT_ >= 0.42f) {
            marchT_ = 0;
            tick_ = 0.05f;
        }
    }

    for (Bay& b : bay_) b.load = 0;
    for (const Member& m : col_) {
        if (m.fell) continue;
        int b = bayAt(m.x);
        if (b >= 0) bay_[size_t(b)].load += m.load;
    }

    phaseT_ += kDt;
    Phase was = phase_;
    if (phase_ == Phase::Gap && phaseT_ > kGapT) {
        int b = pickGust();
        if (b < 0) phaseT_ = kGapT;
        else {
            gustBay_ = b;
            phase_ = Phase::Warn;
            phaseT_ = 0;
        }
    } else if (phase_ == Phase::Warn && phaseT_ > kWarnT) {
        phase_ = Phase::Blow;
        phaseT_ = 0;
    } else if (phase_ == Phase::Blow && phaseT_ > kBlowT) {
        phase_ = Phase::Gap;
        phaseT_ = 0;
        held_++;
        gustN_++;
        gustBay_ = -1;
    }
    if (phase_ == Phase::Blow && was != Phase::Blow) {
        sys_->apu.noiseBurst(0.28f, 640.f, 0.16f);
        shake_ = 1.5f;
        if (!bot_) sys_->rumble(0.35f, 0.12f, 80);
    }

    int here = bayAt(px_);
    bool wasIn = inZone_;
    inZone_ = px_ >= kAnswer;
    hauling_ = bell_ && haul && inZone_;
    bracing_ = brace && here >= 0 && !hauling_;
    if (bell_ && inZone_ && !wasIn) {
        blip_ = 0.07f;
        blipF_ = 988.f;
        if (!bot_) sys_->rumble(0.12f, 0.04f, 40);
    }

    for (int i = 0; i < kBays; i++) {
        Bay& b = bay_[size_t(i)];
        float eq = kBase + kWeight * b.load;
        if (phase_ == Phase::Blow && gustBay_ == i) eq += kGust;
        float pull = (bracing_ && here == i) ? kHold : 0.f;
        b.sag += (kSpring * (eq - b.sag) - pull) * kDt;
        if (b.sag < 0.f) b.sag = 0.f;
        if (b.sag > 1.2f) b.sag = 1.2f;
    }

    int hot = -1;
    for (int i = 0; i < kBays; i++)
        if (bay_[size_t(i)].sag > kBreak) hot = i;
    if (hot >= 0) {
        for (Member& m : col_) {
            if (!m.fell && bayAt(m.x) == hot) {
                m.fell = true;
                m.drop = 12.f;
            }
        }
        loseWatch("THE SPAN GAVE WAY");
        return;
    }

    if (!bell_ && watch_ >= kBellAt) {
        bell_ = true;
        bellTick_ = 0;
        peal_ = true;
        shake_ = 0.6f;
        if (!bot_) sys_->rumble(0.2f, 0.4f, 120);
    }
    if (bell_) {
        bellTick_ += kDt;
        if (bellTick_ >= 0.52f) {
            bellTick_ = 0;
            peal_ = true;
        }
    }
    if (hauling_) rope_ += kDt;
    if (bell_ && rope_ >= kRope - 0.001f) {
        winWatch();
        return;
    }
    if (bell_ && watch_ >= kBellEnd && rope_ < kRope) {
        loseWatch("THE BELL WENT UNANSWERED");
        return;
    }

    if (bracing_ && (int(watch_ * 60.f) % 4) == 0) motes_.push_back({px_, footY(px_) - 4.f, 0.35f});
    for (Mote& m : motes_) {
        m.a -= kDt;
        m.y -= 10.f * kDt;
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.a <= 0.f; }), motes_.end());
    if (motes_.size() > 12) motes_.erase(motes_.begin(), motes_.end() - 12);
    if (shake_ > 0.02f) shake_ *= 0.90f;
    else shake_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += kDt;
    if (mode_ == Mode::Victory || mode_ == Mode::Over) endT_ += kDt;

    float dir = 0;
    bool brace = false, haul = false, go = false;
    if (bot_) botIntent(dir, brace, haul, go);
    else readPad(dir, brace, haul, go);

    if (go) {
        if (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Over) begin();
        else if (mode_ == Mode::Watch) mode_ = Mode::Pause;
        else if (mode_ == Mode::Pause) mode_ = Mode::Watch;
    }

    if (mode_ == Mode::Title) {
        for (int i = 0; i < kBays; i++) bay_[size_t(i)].sag = 0.14f + 0.04f * std::sin(t_ * 1.2f + float(i));
        px_ = 150.f + std::sin(t_ * 0.7f) * 18.f;
        bracing_ = hauling_ = false;
    } else if (mode_ == Mode::Watch) {
        stepWatch(dir, brace, haul);
    } else if (mode_ == Mode::Over) {
        for (Member& m : col_)
            if (m.fell) m.drop += 150.f * kDt;
        if (shake_ > 0.02f) shake_ *= 0.90f;
    } else if (mode_ == Mode::Pause) {
        bracing_ = hauling_ = false;
    }

    draw();
    audio();

    if (mode_ == Mode::Victory) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Over) sys.setLight(190, 30, 24);
    else if (hauling_) sys.setLight(220, 170, 40);
    else if (bell_) sys.setLight(170, 110, 30);
    else if (bracing_) sys.setLight(180, 130, 40);
    else sys.setLight(30, 40, 70);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(std::lround(w), 1L, 400L));
    s.h = int16_t(std::clamp(std::lround(h), 1L, 400L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
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

    float dawn = 0;
    if (mode_ == Mode::Victory || bell_) dawn = 1.f;
    else if (mode_ == Mode::Watch || mode_ == Mode::Pause)
        dawn = std::clamp((watch_ - (kBellAt - 3.f)) / 3.f, 0.f, 1.f);
    const uint16_t skyTop = lerpC(gs::rgb4(1, 2, 6), gs::rgb4(4, 3, 6), dawn);
    const uint16_t skyMid = lerpC(gs::rgb4(2, 4, 8), gs::rgb4(10, 6, 5), dawn);
    const uint16_t hor = lerpC(gs::rgb4(6, 5, 6), gs::rgb4(14, 8, 4), dawn);
    const uint16_t gorge = gs::rgb4(2, 2, 3);
    const uint16_t water0 = lerpC(gs::rgb4(1, 3, 5), gs::rgb4(3, 5, 7), dawn);
    const uint16_t water1 = gs::rgb4(1, 2, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) c = lerpC(skyTop, skyMid, y / 78.f);
        else if (y < 118) c = lerpC(skyMid, hor, (y - 78) / 40.f);
        else if (y < 168) c = lerpC(hor, gorge, (y - 118) / 50.f);
        else c = lerpC(water0, water1, (y - 168) / 56.f);
        vdp.lineBackdrop[y] = c;
    }

    float mx = 0;
    for (const Bay& b : bay_) mx = std::max(mx, b.sag);
    float jig = shake_ + (mx > 0.64f && mode_ == Mode::Watch ? (mx - 0.64f) * 4.f : 0.f);
    float ox = std::sin(t_ * 90.f) * jig;
    float oy = std::cos(t_ * 70.f) * jig * 0.35f;

    if (mode_ == Mode::Title) spr(art_.title, 160, 16, 28, PAL_TITLE);
    else if (mode_ == Mode::Victory) spr(art_.held, 160, 16, 30, PAL_GOOD);
    else if (mode_ == Mode::Over) spr(rope_ < 0.2f && !bell_ ? art_.broke : art_.late, 160, 16, 30, PAL_ALERT);
    else if (mode_ == Mode::Pause) spr(art_.paused, 160, 16, 26, PAL_AMBER);

    bool gustShow = gustBay_ >= 0 && phase_ != Phase::Gap && (mode_ == Mode::Watch || mode_ == Mode::Pause);
    if (gustShow && !(phase_ == Phase::Warn && (int(t_ * 10.f) & 1))) {
        float cx = bayCenter(gustBay_) + ox;
        float cy = footY(bayCenter(gustBay_)) - 46.f + oy;
        spr(art_.chev, cx, cy, phase_ == Phase::Blow ? 16.f : 12.f, PAL_ALERT);
        if (phase_ == Phase::Blow) {
            for (int s = 0; s < 3; s++) {
                float sx = cx - 22.f + std::fmod(t_ * 80.f + float(s) * 12.f, 40.f);
                spr(art_.streak, sx, cy + 8.f + float(s) * 4.f, 4.f, PAL_HEAT);
            }
        }
    }
    for (const Mote& m : motes_) spr(art_.dust, m.x + ox, m.y + oy, 7.f + (0.35f - m.a) * 8.f, PAL_DECK);

    int pose = (bracing_ || hauling_) ? 1 : 0;
    spr(art_.keeper[pose], px_ + ox, footY(px_) + oy, pose ? 36.f : 38.f, PAL_KEEPER, faceLeft_, 0, true);

    float swing = std::sin(t_ * (bell_ || mode_ == Mode::Victory ? 8.5f : 1.4f)) * (bell_ || mode_ == Mode::Victory ? 11.f : 2.2f);
    float lift = std::min(rope_ / kRope, 1.f) * 10.f;
    float yokeY = 42.f + oy;
    spr(art_.yoke, kBellX + ox, yokeY, 8.f, PAL_DECK);
    spr(art_.bell, kBellX + swing + ox, yokeY + 16.f - lift, 22.f, PAL_BELL);
    float ropeH = 40.f - lift * 0.6f;
    spr(art_.rope, kBellX + swing * 0.35f + ox, yokeY + 28.f - lift * 0.3f, ropeH,
        (bell_ && inZone_) ? PAL_HEAT : PAL_DECK);

    for (int i = 0; i < kColumn; i++) {
        const Member& m = col_[size_t(i)];
        int fr = m.x + 0.4f < m.station ? (int(m.x / 7.f) & 1) : (int(t_ * 2.f) & 1);
        float y = footY(m.x) + m.drop + oy;
        float x = m.x + ox + (m.fell ? std::sin(m.drop * 0.12f) * 6.f : 0.f);
        if (m.kind == Kind::Drum) spr(art_.drum[fr], x, y, 38.f, PAL_COAT, false, 0, true);
        else if (m.kind == Kind::Wagon) spr(art_.wagon[fr], x, y, 28.f, PAL_WAGON, false, 0, true);
        else spr(art_.coat[fr], x, y, 36.f, PAL_COAT, false, 0, true);
    }

    int here = bayAt(px_);
    for (int i = 0; i < kBays; i++) {
        float surface = kDeck + bay_[size_t(i)].sag * kSagPx + oy;
        int pal = bay_[size_t(i)].sag > 0.70f ? PAL_HEAT : PAL_DECK;
        spr(art_.plank, bayCenter(i) + ox, surface + 5.f, 13.f, pal);
    }
    spr(art_.lip, 46 + ox, kDeck + 6.f + oy, 16.f, PAL_STONE);
    spr(art_.lip, 286 + ox, kDeck + 6.f + oy, 16.f, PAL_STONE);

    for (int i = 0; i < kBays; i++) {
        float u = (bayCenter(i) - kSpanL) / (kSpanR - kSpanL);
        float y0 = cableY(u) + oy;
        float y1 = kDeck + bay_[size_t(i)].sag * kSagPx + oy;
        float h = std::max(8.f, y1 - y0);
        const gs::Mipped* img = &art_.hanger;
        int pal = PAL_DECK;
        if (bay_[size_t(i)].sag > 0.62f) {
            img = &art_.hot;
            pal = PAL_HEAT;
        } else if (bracing_ && here == i) {
            img = &art_.taut;
        }
        spr(*img, bayCenter(i) + ox, (y0 + y1) * 0.5f, h, pal);
    }
    for (int i = 0; i <= 14; i++) {
        float u = float(i) / 14.f;
        float x = kSpanL + (kSpanR - kSpanL) * u;
        spr(art_.link, x + ox, cableY(u) + oy, 7.f, PAL_DECK);
    }

    int ff = int(t_ * 8.f) & 1;
    spr(art_.flame[ff], 50 + ox, 58 + oy, 12.f, PAL_NIGHT);
    spr(art_.flame[ff], kBellX + 16.f + ox, 56 + oy, 11.f, PAL_NIGHT);
    spr(art_.pennant, 38 + std::sin(t_ * 3.f) * 2.f + ox, 64 + oy, 12.f, PAL_ALERT);

    spr(art_.pier, 50 + ox, 168 + oy, 124.f, PAL_STONE, false, 0, true);
    spr(art_.belfry, kBellX + ox, 176 + oy, 138.f, PAL_STONE, false, 0, true);

    spr(art_.cliff, 18 + ox, 228 + oy, 150.f, PAL_STONE, false, 1, true);
    spr(art_.cliff, 304 + ox, 228 + oy, 150.f, PAL_STONE, true, 1, true);
    spr(art_.wall, 160 + ox, 196 + oy, 96.f, PAL_STONE, false, 8, true);

    if (dawn < 0.9f) spr(art_.moon, 196, 26, 18.f, PAL_NIGHT, false, int(dawn * 10.f));
    spr(art_.cloud, std::fmod(30.f + t_ * 7.f, 440.f) - 60.f, 20.f, 18.f, PAL_NIGHT, false, 3);
    spr(art_.cloud, std::fmod(180.f + t_ * 4.f, 480.f) - 70.f, 32.f, 22.f, PAL_NIGHT, false, 4);
    spr(art_.bird, std::fmod(t_ * 18.f, 400.f) - 30.f, 48.f + std::sin(t_ * 2.f) * 3.f, 8.f, PAL_NIGHT);
    spr(art_.bird, std::fmod(t_ * 12.f + 160.f, 420.f) - 40.f, 36.f, 6.f, PAL_NIGHT, true);
    for (int i = 0; i < 6; i++) {
        float x = 90.f + float(i) * 24.f + std::sin(t_ * 1.3f + float(i)) * 4.f;
        float y = 186.f + float(i % 3) * 8.f + std::sin(t_ * 2.1f + float(i)) * 2.f;
        spr(art_.glint, x, y, 4.f, PAL_NIGHT);
    }

    if (mode_ == Mode::Title) {
        hudC(18, "YOU HAVE THE SPAN", PAL_AMBER);
        hudC(19, "HOLD IT UNTIL THE RELIEF BELL", PAL_HUD);
        hudC(20, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(22, "ARROWS MOVE ALONG THE DECK", PAL_DIM);
        hudC(23, "SPACE OR Z BRACES THE BAY", PAL_DIM);
        hudC(24, "UP AT THE FAR TOWER HAULS", PAL_DIM);
        if ((int(t_ * 2.f) & 1) == 0) hudC(25, "ENTER TAKES THE WATCH", PAL_GOOD);
        hud(1, 27, S3_VERSION_STRING, PAL_DIM);
        hud(34, 27, "S3-16", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hudC(18, "THE SPAN HELD", PAL_GOOD);
        hudC(19, "UNTIL THE RELIEF BELL", PAL_AMBER);
        char buf[40];
        std::snprintf(buf, sizeof buf, "SCORE %d", score());
        hudC(21, buf, PAL_HUD);
        hudC(24, "ENTER TO TAKE THE WATCH AGAIN", PAL_DIM);
    } else if (mode_ == Mode::Over) {
        hudC(18, reason_, PAL_ALERT);
        if (bell_ || rope_ > 0.f) hudC(19, "SURVIVING THE WATCH IS NOT RELIEF", PAL_HUD);
        else hudC(19, "THE COLUMN IS IN THE GORGE", PAL_HUD);
        hudC(24, "ENTER TO TRY THE SPAN AGAIN", PAL_DIM);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "ENTER TO GO ON", PAL_HUD);
    }

    if (mode_ == Mode::Watch || mode_ == Mode::Pause) {
        hud(1, 0, "S3 SPAN", PAL_TITLE);
        char buf[48];
        std::snprintf(buf, sizeof buf, "HELD %d", held_);
        hud(32, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SC %d", score());
        hud(24, 0, buf, PAL_DIM);
        if (!bell_) std::snprintf(buf, sizeof buf, "WATCH %d", int(watch_));
        else std::snprintf(buf, sizeof buf, "BELL %d", int(std::max(0.f, kBellEnd - watch_) + 0.99f));
        hud(1, 1, buf, bell_ ? PAL_AMBER : PAL_HUD);

        int deck = 0;
        for (const Member& m : col_)
            if (!m.fell && m.x >= kSpanL && m.x <= kSpanR) deck++;
        std::snprintf(buf, sizeof buf, "DECK %d/%d", deck, kColumn);
        hud(30, 1, buf, deck == kColumn ? PAL_GOOD : PAL_DIM);

        std::string meter = "BAY ";
        int hotPal = PAL_HUD;
        for (int i = 0; i < kBays; i++) {
            bool g = i == gustBay_ && phase_ != Phase::Gap;
            if (g) meter.push_back('[');
            meter.push_back(strainMark(bay_[size_t(i)].sag));
            if (g) meter.push_back(']');
            else meter.push_back(' ');
            if (bay_[size_t(i)].sag >= 0.70f) hotPal = PAL_ALERT;
        }
        hud(1, 26, meter, hotPal);

        if (bell_) {
            int n = int(std::min(rope_ / kRope, 1.f) * 10.f + 0.001f);
            std::string bar = "ROPE ";
            for (int i = 0; i < 10; i++) bar.push_back(i < n ? '#' : '-');
            hud(28, 25, bar, inZone_ ? PAL_GOOD : PAL_AMBER);
        }

        const char* msg = "HOLD THE SPAN";
        int mp = PAL_HUD;
        if (mx >= 0.70f) {
            msg = "THE SPAN IS GOING";
            mp = PAL_ALERT;
        } else if (phase_ == Phase::Blow) {
            msg = "BRACE THIS BAY";
            mp = PAL_ALERT;
        } else if (phase_ == Phase::Warn) {
            msg = "GUST ON THE SPAN";
            mp = PAL_AMBER;
        } else if (bell_ && inZone_) {
            msg = "HOLD UP TO HAUL";
            mp = PAL_GOOD;
        } else if (bell_) {
            msg = "THE BELL - FAR TOWER";
            mp = PAL_AMBER;
        } else if (watch_ > kBellAt - 2.5f) {
            msg = "RELIEF IS CLOSE";
            mp = PAL_AMBER;
        }
        hudC(25, msg, mp);
        hud(1, 27, "ARROWS MOVE  Z BRACES  UP HAULS", PAL_DIM);
    }
}

void Game::audio() {
    gs::APU& a = sys_->apu;
    if (peal_) {
        peal_ = false;
        a.keyOn(0, 698.f, 0.20f);
        a.keyOn(1, 1046.f, 0.08f);
    }
    if (mode_ == Mode::Victory) {
        static const float notes[] = {440.f, 554.f, 659.f, 880.f, 659.f, 880.f, 1174.f, 880.f};
        int step = int(endT_ * 6.f) % 8;
        a.tone(0, notes[step], 0.055f);
        a.tone(1, notes[(step + 2) % 8] * 0.5f, 0.03f);
        a.tone(2, 0, 0);
        a.noise(0, 0, false);
        return;
    }
    if (blip_ > 0.f) {
        blip_ -= kDt;
        a.tone(0, blipF_, 0.06f);
    } else if (tick_ > 0.f) {
        tick_ -= kDt;
        a.tone(0, 196.f, 0.04f);
    } else {
        a.tone(0, 0, 0);
    }
    if (mode_ == Mode::Watch) {
        float mx = 0;
        for (const Bay& b : bay_) mx = std::max(mx, b.sag);
        if (mx > 0.34f) a.tone(1, 46.f + mx * 36.f, 0.02f + mx * 0.035f);
        else a.tone(1, 0, 0);
        if (bracing_ || hauling_) a.tone(2, hauling_ ? 246.f : 174.f, 0.035f);
        else a.tone(2, 0, 0);
        if (phase_ == Phase::Blow) a.noise(0.045f, 520.f, false);
        else a.noise(0.01f, 240.f, false);
    } else if (mode_ == Mode::Over && endT_ < 0.35f) {
        a.tone(1, 62.f, 0.07f);
        a.tone(2, 0, 0);
        a.noise(0.02f, 180.f, false);
    } else {
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
        a.noise(0, 0, false);
    }
}

}  // namespace spanreli

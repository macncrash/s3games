#include "game/span.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace span {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kBays = 5;
constexpr float kSpanL = 78.f;
constexpr float kSpanR = 242.f;
constexpr float kDeck = 108.f;
constexpr float kSagPx = 42.f;
constexpr float kBreak = 0.78f;
constexpr float kWalk = 24.f;
constexpr float kRun = 200.f;
constexpr float kSpring = 2.10f;
constexpr float kHold = 1.85f;
constexpr float kGust = 0.58f;
constexpr float kBase = 0.02f;
constexpr float kWeight = 0.30f;
constexpr float kSpace = 36.f;
constexpr float kWarnT = 0.80f;
constexpr float kBlowT = 1.05f;
constexpr float kGapT = 1.35f;
constexpr float kX0 = 112.f;
constexpr float kPi = 3.1415926f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

char strainMark(float s) {
    if (s >= 0.70f) return '#';
    if (s >= 0.55f) return '!';
    if (s >= 0.40f) return '+';
    if (s >= 0.22f) return ':';
    return '.';
}

}  // namespace

void Game::boot() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    across_ = 0;
    playT_ = 0;
    clearT_ = 0;
    phase_ = Phase::Gap;
    phaseT_ = 0;
    gustBay_ = -1;
    gustN_ = 0;
    goal_ = 0;
    fan_ = -1;
    splash_ = false;
    shake_ = 0;
    blip_ = 0;
    holding_ = false;
    motes_.clear();
    begin();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    playT_ = 0;
    clearT_ = 0;
}

void Game::begin() {
    Kind kinds[kColumn] = {Kind::Banner, Kind::Pike, Kind::Pike, Kind::Pike, Kind::Pike, Kind::Pike, Kind::Cart};
    float loads[kColumn] = {0.85f, 1.f, 1.f, 1.f, 1.f, 1.f, 2.f};
    for (int i = 0; i < kColumn; i++) {
        col_[i].kind = kinds[i];
        col_[i].load = loads[i];
        col_[i].x = kX0 - float(i) * kSpace;
        col_[i].drop = 0;
        col_[i].fell = false;
        col_[i].parked = false;
    }
    for (Bay& b : bay_) {
        b.sag = 0;
        b.load = 0;
    }
    px_ = kX0;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    across_ = 0;
    playT_ = 0;
    clearT_ = 0;
    phase_ = Phase::Gap;
    phaseT_ = 0;
    gustBay_ = -1;
    gustN_ = 0;
    goal_ = bayAt(px_);
    if (goal_ < 0) goal_ = 0;
    fan_ = -1;
    splash_ = false;
    shake_ = 0;
    blip_ = 0;
    motes_.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 7, 8));
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
    float bob = (mode_ == Mode::Title) ? std::sin(t_ * 1.4f) * 1.4f : std::sin(t_ * 2.4f + u * 5.f) * 0.55f;
    return 42.f + std::sin(u * kPi) * (11.f + avg * 16.f) + bob;
}

float Game::parkX(int i) const { return kSpanR + 8.f + float(kColumn - 1 - i) * 7.f; }

int Game::heaviest() const {
    int heavy = -1;
    float best = 0.2f;
    for (int i = 0; i < kBays; i++) {
        float L = bay_[size_t(i)].load;
        if (L > best + 0.01f || (heavy >= 0 && std::fabs(L - best) <= 0.01f && i > heavy)) {
            best = L;
            heavy = i;
        }
    }
    return heavy;
}

int Game::pickGust() const {
    int heavy = heaviest();
    if (heavy < 0) return -1;
    if (gustN_ % 3 == 2) return heavy;
    int dir = (gustN_ % 2 == 0) ? 1 : -1;
    for (int dist = 2; dist >= 1; --dist) {
        int j = heavy + dir * dist;
        if (j >= 0 && j < kBays && bay_[size_t(j)].load > 0.15f) return j;
    }
    for (int dist = 1; dist <= 2; ++dist) {
        int j = heavy - dir * dist;
        if (j >= 0 && j < kBays && bay_[size_t(j)].load > 0.15f) return j;
    }
    return heavy;
}

void Game::botIntent(float& dir, bool& hold, bool& go) {
    dir = 0;
    hold = false;
    go = false;
    if (mode_ != Mode::Play) {
        if (mode_ == Mode::Title && t_ > 0.35f) go = true;
        return;
    }

    int danger = -1;
    float worst = 0.70f;
    for (int i = 0; i < kBays; i++) {
        if (bay_[size_t(i)].load > 0.15f && bay_[size_t(i)].sag >= worst) {
            worst = bay_[size_t(i)].sag;
            danger = i;
        }
    }
    int goal = danger;
    if (goal < 0 && gustBay_ >= 0 && phase_ != Phase::Gap) goal = gustBay_;
    if (goal < 0) goal = heaviest();
    if (goal < 0) goal = bayAt(px_) >= 0 ? bayAt(px_) : 0;
    goal_ = goal;

    float tx = bayCenter(goal_);
    if (px_ < tx - 2.5f) dir = 1.f;
    else if (px_ > tx + 2.5f) dir = -1.f;

    int here = bayAt(px_);
    if (here >= 0 && (here == goal_ || bay_[size_t(here)].sag > 0.36f || (here == gustBay_ && phase_ != Phase::Gap)))
        hold = true;
}

void Game::stepPlay(float dir, bool hold) {
    playT_ += kDt;
    clearT_ = playT_;
    px_ = std::clamp(px_ + dir * kRun * kDt, 16.f, 304.f);

    for (int i = 0; i < kColumn; i++) {
        Member& m = col_[size_t(i)];
        if (m.fell) continue;
        float prev = m.x;
        float cap = parkX(i);
        if (m.x < cap) m.x = std::min(cap, m.x + kWalk * kDt);
        m.parked = m.x >= cap - 0.05f;
        if (prev <= kSpanR && m.x > kSpanR) {
            blip_ = 0.07f;
            blipF_ = 620.f + float(i) * 28.f;
        }
    }

    for (Bay& b : bay_) b.load = 0;
    for (const Member& m : col_) {
        if (m.fell) continue;
        int b = bayAt(m.x);
        if (b >= 0) bay_[size_t(b)].load += m.load;
    }

    Phase was = phase_;
    phaseT_ += kDt;
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
        gustBay_ = -1;
        gustN_++;
    }
    if (phase_ == Phase::Blow && was != Phase::Blow) {
        sys_->apu.noiseBurst(0.30f, 720.f, 0.18f);
        shake_ = 1.4f;
        if (!bot_) sys_->rumble(0.35f, 0.15f, 90);
    }

    int here = bayAt(px_);
    for (int i = 0; i < kBays; i++) {
        Bay& b = bay_[size_t(i)];
        float eq = kBase + kWeight * b.load;
        if (phase_ == Phase::Blow && gustBay_ == i) eq += kGust;
        float pull = (hold && here == i) ? kHold : 0.f;
        b.sag += (kSpring * (eq - b.sag) - pull) * kDt;
        b.sag = std::clamp(b.sag, 0.f, 1.25f);
    }

    across_ = 0;
    bool broken = false;
    int breakBay = -1;
    for (Member& m : col_) {
        if (!m.fell && m.x > kSpanR) across_++;
        if (m.fell) continue;
        int b = bayAt(m.x);
        if (b >= 0 && bay_[size_t(b)].sag > kBreak) {
            m.fell = true;
            m.drop = 8.f;
            broken = true;
            breakBay = b;
        }
    }
    if (broken) {
        mode_ = Mode::Lose;
        over_ = true;
        won_ = false;
        clearT_ = playT_;
        shake_ = 3.2f;
        sys_->apu.noiseBurst(0.55f, 160.f, 0.4f);
        sys_->apu.tone(1, 64.f, 0.09f);
        std::fprintf(stderr, "span break t=%.2f bay %d sag %.2f\n", playT_, breakBay,
                     breakBay >= 0 ? bay_[size_t(breakBay)].sag : 0.f);
        return;
    }

    bool home = true;
    for (int i = 0; i < kColumn; i++) {
        if (col_[size_t(i)].fell || col_[size_t(i)].x + 0.2f < parkX(i)) home = false;
    }
    if (home) {
        mode_ = Mode::Win;
        over_ = true;
        won_ = true;
        across_ = kColumn;
        clearT_ = playT_;
        shake_ = 0;
        if (!bot_) sys_->rumble(0.2f, 0.08f, 120);
    }

    if (hold && (int(playT_ * 60.f) % 5) == 0) motes_.push_back({px_, footY(px_) - 2.f, 0.4f});
    for (Mote& m : motes_) m.a -= kDt;
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Mote& m) { return m.a <= 0.f; }), motes_.end());
    if (motes_.size() > 16) motes_.erase(motes_.begin(), motes_.end() - 16);
    if (shake_ > 0.15f) shake_ *= 0.90f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += kDt;

    float dir = 0;
    bool hold = false;
    bool go = false;
    if (bot_) {
        botIntent(dir, hold, go);
    } else {
        const gs::Pad& p = sys.pad;
        if (p.down(gs::BTN_LEFT)) dir -= 1.f;
        if (p.down(gs::BTN_RIGHT)) dir += 1.f;
        if (dir == 0.f && std::fabs(p.axisX) > 0.25f) dir = std::clamp(p.axisX, -1.f, 1.f);
        hold = p.down(gs::BTN_A) || p.down(gs::BTN_B) || p.down(gs::BTN_C) || p.down(gs::BTN_UP) || p.down(gs::BTN_TURBO);
        bool tap = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
        bool start = p.pressed(gs::BTN_START);
        if (mode_ == Mode::Play || mode_ == Mode::Pause) go = start;
        else go = start || tap;
    }

    if (go) {
        if (mode_ == Mode::Title || mode_ == Mode::Win || mode_ == Mode::Lose) begin();
        else if (mode_ == Mode::Play) mode_ = Mode::Pause;
        else if (mode_ == Mode::Pause) mode_ = Mode::Play;
    }

    if (mode_ == Mode::Play) stepPlay(dir, hold);
    else if (mode_ == Mode::Lose) {
        for (Member& m : col_) {
            if (!m.fell) continue;
            m.drop += 150.f * kDt;
            if (!splash_ && footY(m.x) + m.drop > 190.f) {
                splash_ = true;
                sys.apu.noiseBurst(0.28f, 400.f, 0.22f);
            }
        }
    }

    holding_ = hold && mode_ == Mode::Play;
    draw();
    audio();

    if (mode_ == Mode::Win) sys.setLight(40, 170, 70);
    else if (mode_ == Mode::Lose) sys.setLight(190, 30, 24);
    else if (holding_) sys.setLight(190, 140, 40);
    else sys.setLight(30, 40, 55);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
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
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::word(const gs::Mipped& m, float cx, float cy, float h, int pal) { spr(m, cx, cy, h, pal, false, 0, false); }

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();

    const uint16_t sky0 = gs::rgb4(5, 7, 10);
    const uint16_t sky1 = gs::rgb4(12, 13, 12);
    const uint16_t gorge0 = gs::rgb4(3, 4, 6);
    const uint16_t gorge1 = gs::rgb4(2, 3, 4);
    const uint16_t water0 = gs::rgb4(2, 6, 7);
    const uint16_t water1 = gs::rgb4(1, 2, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 96) c = lerpC(sky0, sky1, y / 96.f);
        else if (y < 128) c = lerpC(sky1, gorge0, (y - 96) / 32.f);
        else if (y < 180) c = lerpC(gorge0, gorge1, (y - 128) / 52.f);
        else c = lerpC(water0, water1, (y - 180) / 44.f);
        vdp.lineBackdrop[y] = c;
    }

    float mx = 0;
    for (const Bay& b : bay_) mx = std::max(mx, b.sag);
    float jig = shake_ + (mx > 0.64f ? (mx - 0.64f) * 5.f : 0.f);
    float ox = std::sin(t_ * 90.f) * jig;
    float oy = std::cos(t_ * 70.f) * jig * 0.35f;

    if (mode_ == Mode::Title) word(art_.title, 160, 24, 34, PAL_TITLE);
    else if (mode_ == Mode::Win) word(art_.across, 160, 26, 32, PAL_GOOD);
    else if (mode_ == Mode::Lose) word(art_.broke, 160, 26, 32, PAL_ALERT);
    else if (mode_ == Mode::Pause) word(art_.paused, 160, 26, 30, PAL_AMBER);

    bool gustShow = gustBay_ >= 0 && phase_ != Phase::Gap && mode_ == Mode::Play;
    if (gustShow) {
        bool blink = phase_ == Phase::Warn && (int(t_ * 10.f) & 1);
        if (!blink) {
            float cx = bayCenter(gustBay_) + ox;
            float cy = footY(bayCenter(gustBay_)) - 50.f + oy;
            spr(art_.chev, cx, cy, phase_ == Phase::Blow ? 18.f : 14.f, PAL_FX);
            if (phase_ == Phase::Blow) {
                for (int s = 0; s < 3; s++) {
                    float sx = cx - 18.f + std::fmod(t_ * 70.f + float(s) * 14.f, 36.f);
                    spr(art_.streak, sx, cy + 10.f + float(s) * 5.f, 4.f, PAL_FX);
                }
            }
        }
    }

    for (const Mote& m : motes_) spr(art_.dust, m.x + ox, m.y + oy, 8.f + (0.4f - m.a) * 10.f, PAL_FX);

    int pose = (holding_ && mode_ == Mode::Play) ? 1 : 0;
    spr(art_.keeper[pose], px_ + ox, footY(px_) + oy, pose ? 36.f : 38.f, PAL_JACK, false, 0, true);

    for (int i = 0; i < kColumn; i++) {
        const Member& m = col_[size_t(i)];
        int fr = int(m.x / 8.f) & 1;
        float y = footY(m.x) + m.drop + oy;
        float x = m.x + ox + (m.fell ? std::sin(m.drop * 0.15f) * 5.f : 0.f);
        if (m.kind == Kind::Banner) {
            int wf = int(t_ * 5.f) & 1;
            spr(art_.banner[wf], x, y, 40.f, PAL_COAT, false, 0, true);
        } else if (m.kind == Kind::Cart) {
            spr(art_.cart[fr], x, y, 30.f, PAL_CART, false, 0, true);
        } else {
            spr(art_.march[fr], x, y, 36.f, PAL_COAT, false, 0, true);
        }
    }

    spr(art_.pennant, 304 + ox, kDeck + oy, 20.f, PAL_COAT, false, 0, true);
    int ff = int(t_ * 8.f) & 1;
    spr(art_.flame[ff], 268 + ox, 34 + oy, 14.f, PAL_FX);

    for (int i = 0; i < kBays; i++) {
        float cx = bayCenter(i) + ox;
        float surface = kDeck + bay_[size_t(i)].sag * kSagPx + oy;
        spr(art_.plank, cx, surface + 6.f, 13.f, PAL_WOOD);
    }
    spr(art_.lip, 52 + ox, kDeck + 8.f + oy, 16.f, PAL_ROCK);
    spr(art_.lip, 268 + ox, kDeck + 8.f + oy, 16.f, PAL_ROCK, true);

    int here = bayAt(px_);
    for (int i = 0; i < kBays; i++) {
        float u = (bayCenter(i) - kSpanL) / (kSpanR - kSpanL);
        float y0 = cableY(u) + oy;
        float y1 = kDeck + bay_[size_t(i)].sag * kSagPx + oy;
        float h = std::max(6.f, y1 - y0);
        bool taut = holding_ && here == i && mode_ == Mode::Play;
        spr(taut ? art_.taut : art_.rope, bayCenter(i) + ox, (y0 + y1) * 0.5f, h, PAL_ROPE);
    }
    for (int i = 0; i <= 14; i++) {
        float u = float(i) / 14.f;
        float x = kSpanL + (kSpanR - kSpanL) * u;
        spr(art_.link, x + ox, cableY(u) + oy, 7.f, PAL_ROPE);
    }

    spr(art_.tower, 56 + ox, 156 + oy, 118.f, PAL_WOOD, false, 0, true);
    spr(art_.tower, 264 + ox, 156 + oy, 118.f, PAL_WOOD, true, 0, true);

    for (int i = 0; i < 5; i++) {
        float x = 100.f + float(i) * 26.f + std::sin(t_ * 1.4f + float(i)) * 5.f;
        float y = 198.f + float(i % 2) * 8.f + std::sin(t_ * 2.f + float(i) * 1.3f) * 2.f;
        spr(art_.glint, x + ox, y, 5.f, PAL_FX);
    }

    spr(art_.cliff, 30 + ox, 228 + oy, 158.f, PAL_ROCK, false, 0, true);
    spr(art_.cliff, 290 + ox, 228 + oy, 158.f, PAL_ROCK, true, 0, true);
    spr(art_.bird, std::fmod(t_ * 16.f, 390.f) - 30.f, 46.f + std::sin(t_ * 2.f) * 3.f, 8.f, PAL_FX);
    spr(art_.bird, std::fmod(t_ * 11.f + 140.f, 390.f) - 30.f, 34.f, 6.f, PAL_FX, true);
    spr(art_.cloud, std::fmod(30.f + t_ * 6.f, 440.f) - 60.f, 28.f, 22.f, PAL_FX, false, 3);
    spr(art_.cloud, std::fmod(200.f + t_ * 4.f, 480.f) - 70.f, 16.f, 26.f, PAL_FX, false, 2);
    spr(art_.wall, 160 + ox, 172 + oy, 108.f, PAL_ROCK, false, 8, true);
    spr(art_.sun, 286, 22, 22.f, PAL_FX, false, 1);

    if (mode_ == Mode::Title) {
        hudC(19, "HOLD THE SPAN", PAL_AMBER);
        hudC(20, "UNTIL THE COLUMN IS ACROSS", PAL_HUD);
        hudC(22, "ARROWS MOVE ALONG THE SPAN", PAL_HUD);
        hudC(23, "SPACE OR Z HOLDS THE BAY", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(24, "START TO SEND THEM ACROSS", PAL_GOOD);
        hud(1, 27, S3_VERSION_STRING, PAL_HUD);
        hud(34, 27, "S3-16", PAL_AMBER);
    } else if (mode_ == Mode::Win) {
        hudC(16, "THE COLUMN IS ACROSS", PAL_GOOD);
        hudC(17, "THE SPAN HELD", PAL_AMBER);
        hudC(23, "START TO HOLD IT AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        hudC(16, "THE SPAN GAVE WAY", PAL_ALERT);
        hudC(17, "THE COLUMN IS LOST", PAL_HUD);
        hudC(23, "START TO TRY AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(23, "START TO GO ON", PAL_HUD);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        hud(1, 0, "S3 SPAN", PAL_TITLE);
        std::string ac = "ACROSS " + std::to_string(across_) + "/" + std::to_string(kColumn);
        hud(40 - int(ac.size()), 0, ac, across_ == kColumn ? PAL_GOOD : PAL_HUD);

        std::string meter = "BAY ";
        int hot = PAL_HUD;
        for (int i = 0; i < kBays; i++) {
            bool g = i == gustBay_ && phase_ != Phase::Gap;
            if (g) meter.push_back('[');
            meter.push_back(strainMark(bay_[size_t(i)].sag));
            if (g) meter.push_back(']');
            else meter.push_back(' ');
            if (bay_[size_t(i)].sag >= 0.70f) hot = PAL_ALERT;
        }
        hud(1, 26, meter, hot);
        if (mode_ == Mode::Play && mx >= 0.70f) hudC(25, "THE SPAN IS GOING", PAL_ALERT);
        else if (mode_ == Mode::Play && phase_ == Phase::Blow) hudC(25, "HOLD THE BAY", PAL_ALERT);
        else if (mode_ == Mode::Play && phase_ == Phase::Warn) hudC(25, "GUST INCOMING", PAL_AMBER);
        hud(1, 27, "ARROWS MOVE    SPACE OR Z HOLDS", PAL_HUD);
    }
}

void Game::audio() {
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Win) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f, 659.f, 784.f, 1046.f, 784.f};
        int step = int(t_ * 7.f) % 8;
        a.tone(0, notes[step], 0.055f);
        a.tone(1, notes[(step + 2) % 8] * 0.5f, 0.03f);
        a.tone(2, 0, 0);
        return;
    }
    if (blip_ > 0.f) {
        blip_ -= kDt;
        a.tone(0, blipF_, 0.06f);
    } else {
        a.tone(0, 0, 0);
    }
    if (mode_ == Mode::Play) {
        float mx = 0;
        for (const Bay& b : bay_) mx = std::max(mx, b.sag);
        if (mx > 0.34f) a.tone(1, 52.f + mx * 40.f, 0.025f + mx * 0.04f);
        else a.tone(1, 0, 0);
        if (holding_) a.tone(2, 196.f, 0.04f);
        else a.tone(2, 0, 0);
        if (phase_ == Phase::Blow) a.noise(0.04f, 500.f, false);
        else a.noise(0.012f, 280.f, false);
    } else {
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
        a.noise(0, 0, false);
    }
}

}  // namespace span

#include "game/siege.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace siege {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float CONTACT = 128.0f;
constexpr float SPAWN_Y = 258.0f;
constexpr float FLIGHT_L = 216.0f * DT;  // 3.6 s, so a pair can be split
constexpr float FLIGHT_H = 264.0f * DT;  // 4.4 s, time to plant for iron
constexpr float LIGHT_V = (SPAWN_Y - CONTACT) / FLIGHT_L;
constexpr float HEAVY_V = (SPAWN_Y - CONTACT) / FLIGHT_H;
constexpr float SPEED = 210.0f;
constexpr float STONE_V = 260.0f;
constexpr float X_MIN = 52.0f;
constexpr float X_MAX = 268.0f;
constexpr float BODY_HALF = 30.0f;
constexpr float BRACE_HALF = 46.0f;
constexpr float RAM_H = 52.0f;
constexpr int GATE_MAX = 5;

// Spawn frame is counted only while the siege is running. Lights take FLIGHT_L.
struct Order {
    int frame;
    float x;
    bool heavy;
};
constexpr Order kOrders[] = {
    {40, 90.0f, false},
    {300, 228.0f, false},
    {560, 160.0f, true},
    {900, 72.0f, false},
    {900, 248.0f, false},
    {1200, 196.0f, true},
    {1540, 80.0f, false},
    {1540, 240.0f, false},
};
constexpr int kRamCount = int(sizeof kOrders / sizeof kOrders[0]);

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 1, 3));
    mode_ = Mode::Title;
    titleT_ = 0;
    anim_ = 0;
    over_ = false;
    won_ = false;
    through_ = false;
    gate_ = GATE_MAX;
    px_ = 160;
    rams_.clear();
    puffs_.clear();
    stone_ = false;
    quiet();
}

int Game::marker() const {
    if (won_ || mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Title) return 0;
    return 1;
}

void Game::quiet() {
    if (!sys_) return;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noise(0, 0, false);
    fanOn_ = false;
}

void Game::blip(float freq, float vol) {
    sys_->apu.tone(2, freq, vol);
    beep_ = std::max(beep_, 0.07f);
}

void Game::note(const char* s, int pal) {
    note_ = s;
    notePal_ = pal;
    noteT_ = 0.85f;
}

void Game::newSiege() {
    mode_ = Mode::Siege;
    held_ = Mode::Siege;
    siegeFrame_ = 0;
    next_ = 0;
    spawned_ = 0;
    resolved_ = 0;
    stopped_ = 0;
    breaches_ = 0;
    gate_ = GATE_MAX;
    score_ = 0;
    px_ = 160;
    braced_ = false;
    stone_ = false;
    stoneCd_ = 0.25f;
    botDetour_ = -1;
    through_ = false;
    won_ = false;
    over_ = false;
    bannerT_ = 0;
    flash_ = 0;
    shake_ = 0;
    fanOn_ = false;
    fanStep_ = -1;
    noteT_ = 0;
    rams_.clear();
    puffs_.clear();
    blip(520, 0.08f);
}

void Game::spawnDue() {
    while (next_ < kRamCount && siegeFrame_ >= kOrders[next_].frame) {
        Ram r;
        r.heavy = kOrders[next_].heavy;
        r.x = kOrders[next_].x;
        r.y = SPAWN_Y;
        r.speed = r.heavy ? HEAVY_V : LIGHT_V;
        r.half = r.heavy ? 20.0f : 16.0f;
        r.age = 0;
        r.alive = true;
        rams_.push_back(r);
        if (r.heavy) {
            sys_->apu.tone(1, 74, 0.16f);
            beep_ = std::max(beep_, 0.2f);
            note("IRON", PAL_ALERT);
        }
        next_++;
        spawned_ = next_;
    }
}

int Game::stoneVictim() const {
    if (!stone_) return -1;
    int hit = -1;
    float best = 1.0e9f;
    for (int i = 0; i < int(rams_.size()); i++) {
        const Ram& r = rams_[size_t(i)];
        if (!r.alive) continue;
        if (stoneY_ > r.y + RAM_H + 8.0f) continue;
        if (std::fabs(stoneX_ - r.x) > r.half + 12.0f) continue;
        if (r.y < best) {
            best = r.y;
            hit = i;
        }
    }
    return hit;
}

bool Game::afford(int sec, int pri) const {
    const Ram& s = rams_[size_t(sec)];
    const Ram& p = rams_[size_t(pri)];
    float tti = (p.y - CONTACT) / p.speed;
    float toSec = std::fabs(px_ - s.x) / SPEED;
    float back = std::fabs(s.x - p.x) / SPEED;
    // The margin stays flat while we walk: both the detour and the clock shrink at 1 s/s.
    return toSec + 0.22f + back < tti - 0.40f;
}

void Game::botThink(float& slide, bool& brace, bool& drop) {
    slide = 0;
    brace = false;
    drop = false;
    struct Threat {
        int i;
        float tti;
    };
    std::vector<Threat> threats;
    int victim = stoneVictim();
    for (int i = 0; i < int(rams_.size()); i++) {
        if (!rams_[size_t(i)].alive || i == victim) continue;
        float tti = (rams_[size_t(i)].y - CONTACT) / rams_[size_t(i)].speed;
        threats.push_back({i, tti});
    }
    if (threats.empty()) return;
    std::sort(threats.begin(), threats.end(), [](const Threat& a, const Threat& b) {
        if (a.tti != b.tti) return a.tti < b.tti;
        return a.i < b.i;
    });
    int primary = threats[0].i;
    bool stoneFree = !stone_ && stoneCd_ <= 0;
    bool stoning = false;
    int target = primary;
    if (!stoneFree) {
        botDetour_ = -1;
    } else if (botDetour_ >= 0 && botDetour_ != primary && botDetour_ < int(rams_.size()) &&
               rams_[size_t(botDetour_)].alive && afford(botDetour_, primary)) {
        target = botDetour_;
        stoning = true;
    } else {
        botDetour_ = -1;
        for (size_t k = 1; k < threats.size(); k++) {
            int sec = threats[k].i;
            if (!afford(sec, primary)) continue;
            botDetour_ = sec;
            target = sec;
            stoning = true;
            break;
        }
    }

    float step = SPEED * DT;
    float err = rams_[size_t(target)].x - px_;
    bool willBrace = false;
    if (!stoning && rams_[size_t(primary)].heavy) {
        float tti = (rams_[size_t(primary)].y - CONTACT) / rams_[size_t(primary)].speed;
        if (std::fabs(err) < 18.0f && tti < 0.8f) willBrace = true;
    }
    if (willBrace) {
        slide = 0;
        brace = true;
    } else if (std::fabs(err) <= step) {
        slide = step > 0 ? err / step : 0;
    } else {
        slide = std::copysign(1.0f, err);
    }
    if (stoning && std::fabs(err) < 24.0f) drop = true;
}

void Game::tryDrop() {
    if (stone_ || stoneCd_ > 0) return;
    stone_ = true;
    stoneX_ = px_;
    stoneY_ = CONTACT - 8.0f;
    stoneCd_ = 0.55f;
    sys_->apu.noiseBurst(0.22f, 2600, 18);
    sys_->apu.tone(2, 280, 0.07f);
    beep_ = std::max(beep_, 0.08f);
}

void Game::boom(float x, bool bad) {
    for (int i = 0; i < 4; i++) {
        if (puffs_.size() > 16) puffs_.erase(puffs_.begin());
        Puff p;
        p.x = x + float((i - 2) * 6);
        p.y = CONTACT + float(i * 3);
        p.t = 0.28f + float(i) * 0.04f;
        p.h = bad ? 14.0f : 10.0f;
        p.spark = bad && (i & 1);
        puffs_.push_back(p);
    }
    if (bad) {
        sys_->apu.noiseBurst(0.42f, 700, 8);
        sys_->apu.tone(1, 58, 0.16f);
        sys_->rumble(0.85f, 0.55f, 140);
    } else {
        sys_->apu.noiseBurst(0.24f, 1500, 14);
        sys_->apu.tone(1, 120, 0.1f);
        sys_->rumble(0.28f, 0.12f, 70);
    }
    beep_ = std::max(beep_, 0.12f);
    shake_ = bad ? 1.0f : 0.45f;
}

void Game::scar(int n) {
    gate_ -= n;
    if (gate_ < 0) gate_ = 0;
    flash_ = 0.4f;
}

void Game::settle() {
    if (mode_ != Mode::Siege) return;
    if (gate_ <= 0) beginLoss();
    else if (spawned_ >= kRamCount && resolved_ >= kRamCount && stopped_ >= kRamCount) beginWin();
    else if (spawned_ >= kRamCount && resolved_ >= kRamCount) beginLoss();
}

void Game::meet(Ram& ram) {
    if (!ram.alive || mode_ != Mode::Siege) return;
    ram.alive = false;
    float half = braced_ ? BRACE_HALF : BODY_HALF;
    bool cover = std::fabs(px_ - ram.x) <= half + ram.half * 0.85f;
    if (cover && (!ram.heavy || braced_)) {
        stopped_++;
        score_ += ram.heavy ? 250 : 100;
        note(ram.heavy ? "+250" : "+100", PAL_GOLD);
        boom(ram.x, false);
    } else if (cover) {
        stopped_++;
        score_ += 60;
        scar(1);
        note("CRACK", PAL_ALERT);
        boom(ram.x, true);
    } else {
        breaches_++;
        through_ = true;
        scar(GATE_MAX);
        note("THROUGH", PAL_ALERT);
        boom(ram.x, true);
    }
    resolved_++;
    settle();
}

void Game::stepStone(float dt) {
    if (!stone_ || mode_ != Mode::Siege) return;
    float y0 = stoneY_;
    stoneY_ += STONE_V * dt;
    int hit = -1;
    float best = 1.0e9f;
    for (int i = 0; i < int(rams_.size()); i++) {
        Ram& r = rams_[size_t(i)];
        if (!r.alive) continue;
        if (std::fabs(stoneX_ - r.x) > r.half + 12.0f) continue;
        float top = r.y;
        float bot = r.y + RAM_H;
        if (stoneY_ < top - 2.0f || y0 > bot) continue;
        if (top < best) {
            best = top;
            hit = i;
        }
    }
    if (hit >= 0) {
        Ram& r = rams_[size_t(hit)];
        r.alive = false;
        stopped_++;
        score_ += 150;
        note("+150", PAL_GOLD);
        boom(r.x, false);
        resolved_++;
        stone_ = false;
        settle();
        return;
    }
    if (stoneY_ > gs::SCREEN_H + 8.0f) stone_ = false;
}

void Game::stepRams(float dt) {
    for (int i = 0; i < int(rams_.size()); i++) {
        Ram& r = rams_[size_t(i)];
        if (!r.alive) continue;
        r.y -= r.speed * dt;
        r.age += dt;
        if (mode_ == Mode::Siege && (siegeFrame_ + i) % 14 == 0) {
            if (puffs_.size() > 16) puffs_.erase(puffs_.begin());
            Puff p;
            p.x = r.x;
            p.y = r.y + 36.0f;
            p.t = 0.22f;
            p.h = 7.0f;
            p.spark = false;
            puffs_.push_back(p);
        }
        if (r.y <= CONTACT) {
            meet(r);
            if (mode_ != Mode::Siege) return;
        }
    }
}

void Game::beginWin() {
    if (mode_ != Mode::Siege) return;
    mode_ = Mode::Won;
    won_ = true;
    through_ = false;
    bannerT_ = 0;
    fanOn_ = true;
    fanStep_ = -1;
    fanT_ = 0;
    sys_->rumble(0.2f, 0.45f, 180);
}

void Game::beginLoss() {
    if (mode_ != Mode::Siege) return;
    mode_ = Mode::Lost;
    won_ = false;
    bannerT_ = 0;
    fanOn_ = false;
    quiet();
    sys_->apu.noiseBurst(0.5f, 280, 5);
    sys_->rumble(1.0f, 0.7f, 220);
    beep_ = 0.3f;
}

void Game::fanfare() {
    static const float notes[] = {392.0f, 523.0f, 659.0f, 784.0f};
    fanT_ += DT;
    int step = int(fanT_ / 0.16f);
    if (step != fanStep_) {
        fanStep_ = step;
        if (step < 4) {
            sys_->apu.tone(0, notes[step], 0.14f);
            beep_ = 0.17f;
        } else {
            sys_->apu.tone(0, 0, 0);
            if (step > 5) fanOn_ = false;
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
            if (!fanOn_) sys.apu.tone(0, 0, 0);
        }
    }
    if (flash_ > 0) flash_ = std::max(0.0f, flash_ - DT);
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT * 1.4f);
    if (stoneCd_ > 0) stoneCd_ = std::max(0.0f, stoneCd_ - DT);
    if (noteT_ > 0) noteT_ -= DT;
    for (Puff& p : puffs_) p.t -= DT;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t <= 0; }), puffs_.end());

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);
    bool tap = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    bool brace = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
    bool drop = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    float slide = 0;
    if (pad.down(gs::BTN_LEFT)) slide -= 1;
    if (pad.down(gs::BTN_RIGHT)) slide += 1;
    if (std::fabs(pad.axisX) > 0.2f) slide = pad.axisX;

    if (bot_) {
        start = false;
        back = false;
        tap = false;
        brace = false;
        drop = false;
        slide = 0;
        if (mode_ == Mode::Title && titleT_ > 0.4f) start = true;
        else if (mode_ == Mode::Siege) botThink(slide, brace, drop);
    }

    if (mode_ == Mode::Title) {
        titleT_ += DT;
        if (back && !bot_) sys.quit();
        else if (start || tap) newSiege();
    } else if (mode_ == Mode::Pause) {
        if (start || tap) mode_ = held_;
        else if (back) {
            mode_ = Mode::Title;
            titleT_ = 0;
            over_ = false;
            won_ = false;
            quiet();
        }
    } else if (mode_ == Mode::Siege) {
        if (start && !bot_) {
            held_ = Mode::Siege;
            mode_ = Mode::Pause;
            quiet();
        } else {
            siegeFrame_++;
            spawnDue();
            if (brace) slide = 0;  // planting the stones: the wall does not creep
            braced_ = brace;
            px_ = clampf(px_ + slide * SPEED * DT, X_MIN, X_MAX);
            if (drop) tryDrop();
            stepStone(DT);
            stepRams(DT);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        bannerT_ += DT;
        braced_ = false;
        if (bannerT_ > 1.1f) over_ = true;
        if ((start || tap) && bannerT_ > 0.35f) newSiege();
        else if (back) {
            mode_ = Mode::Title;
            titleT_ = 0;
            over_ = false;
            won_ = false;
            quiet();
        }
    }
    if (fanOn_) fanfare();
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool top) {
    if (h < 1.0f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + shx_ - s.w * 0.5f));
    float topY = top ? cy : cy - float(s.h) * 0.5f;
    s.y = int16_t(std::lround(topY + shy_));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool shadow) {
    if (w < 1.0f || h < 1.0f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx + shx_ - s.w * 0.5f));
    s.y = int16_t(std::lround(cy + shy_ - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::banner(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    spr(m, cx, cy, h, pal, false, false);
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

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    bool hold = mode_ == Mode::Won;
    bool fell = mode_ == Mode::Lost;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        uint16_t c;
        if (y < 58) {
            float u = float(y) / 58.0f;
            int r = 3 + int(u * 9.0f);
            int g = 2 + int(u * 3.0f);
            int b = 8 - int(u * 5.0f);
            if (b < 2) b = 2;
            if (hold) {
                r = std::min(15, r + 1);
                g = std::min(15, g + 2);
            }
            if (fell) r = std::min(15, r + 4);
            if (flash_ > 0) r = std::min(15, r + int(flash_ * 8.0f));
            c = gs::rgb4(r, g, b);
        } else if (y < int(CONTACT) + 6) {
            int shade = 3 + ((y / 6) & 1);
            if (fell) shade = std::min(6, shade + 1);
            c = gs::rgb4(shade, shade, shade + 1);
        } else {
            float u = float(y - int(CONTACT)) / 100.0f;
            int band = (y / 7) & 1;
            int r = 2 + band + int(u * 2.0f);
            int g = 3 + band + int((1.0f - u) * 2.0f);
            int b = 1;
            if (fell) r = std::min(15, r + 2);
            c = gs::rgb4(r, g, b);
        }
        if (y == int(CONTACT) || y == int(CONTACT) + 1) c = gs::rgb4(5, 4, 3);
        v.lineBackdrop[y] = c;
    }
    if (hold) v.setFogColor(gs::rgb4(3, 3, 4));
    else if (fell) v.setFogColor(gs::rgb4(5, 1, 2));
    else v.setFogColor(gs::rgb4(2, 1, 3));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0;
    if (shake_ > 0) {
        shx_ = std::sin(anim_ * 92.0f) * 4.5f * shake_;
        shy_ = std::cos(anim_ * 74.0f) * 2.5f * shake_;
    }
    backdrop();
    if (mode_ == Mode::Won) sys_->setLight(40, 170, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(190, 30, 24);
    else if (braced_) sys_->setLight(200, 110, 30);
    else sys_->setLight(70, 55, 40);

    int look = 0;
    if (gate_ < GATE_MAX) look = gate_ >= 3 ? 1 : 2;
    if (mode_ == Mode::Lost) look = 2;
    int torch = int(anim_ * 7.0f) & 1;
    float bob = std::sin(anim_ * 2.2f) * 2.0f;

    if (mode_ == Mode::Title) {
        banner(art_.title, 164, 16, float(art_.title.h), PAL_GOLD);
        banner(art_.sub, 160, 38, float(art_.sub.h), PAL_HUD);
    } else if (mode_ == Mode::Won) {
        banner(art_.hold, 160, 18, float(art_.hold.h) + 2.0f, PAL_GOLD);
        banner(art_.sub, 160, 42, float(art_.sub.h) * 0.85f, PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        const gs::Mipped& line = through_ ? art_.through : art_.broke;
        banner(line, 160, 18, float(line.h) + 1.0f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        banner(art_.pause, 160, 20, float(art_.pause.h), PAL_GOLD);
    }

    for (const Puff& p : puffs_) {
        float k = clampf(p.t * 3.0f, 0.25f, 1.0f);
        spr(art_.puff, p.x, p.y, p.h * (p.spark ? k : (1.4f - k)), p.spark ? PAL_FIRE : PAL_DUST, false, false);
    }
    if (stone_) spr(art_.rock, stoneX_, stoneY_, 16, PAL_ROCK, false, true);

    float bodyH = braced_ ? 68.0f : 84.0f;
    const gs::Mipped& body = braced_ ? art_.youBrace : art_.you;
    spr(body, px_, CONTACT - bodyH, bodyH, PAL_WALL, false, true);

    for (const Ram& r : rams_) {
        if (!r.alive) continue;
        int fr = int(r.age * 8.0f) & 1;
        float h = r.heavy ? 62.0f : 52.0f;
        stamp(art_.shadow, r.x, r.y + h - 6.0f, h * 0.7f, 8, PAL_DUST, true);
        spr(r.heavy ? art_.iron[fr] : art_.ram[fr], r.x, r.y, h, r.heavy ? PAL_IRON : PAL_RAM, false, true);
    }
    if (mode_ == Mode::Title) {
        // Sit in the open field, above the title hints.
        stamp(art_.shadow, 206, 180, 32, 8, PAL_DUST, true);
        spr(art_.ram[torch], 206, 140, 40, PAL_RAM, false, true);
        stamp(art_.shadow, 108, 178, 34, 8, PAL_DUST, true);
        spr(art_.iron[1 - torch], 108, 134, 44, PAL_IRON, false, true);
    }

    spr(art_.gate[look], 160, CONTACT - 88, 90, PAL_GATE, false, true);
    for (int i = 0; i < 5; i++) {
        float x = 42.0f + float(i) * 58.0f;
        spr(art_.course, x, CONTACT - 22, 20, PAL_WALL, false, true);
    }
    for (int i = 0; i < 7; i++) {
        float x = 58.0f + float(i) * 34.0f;
        if (std::fabs(x - px_) < 26.0f) continue;
        spr(art_.merlon, x, CONTACT - 78, 40, PAL_WALL, i & 1, true);
    }
    spr(art_.tower, 24, CONTACT - 108, 112, PAL_WALL, false, true);
    spr(art_.tower, 296, CONTACT - 108, 112, PAL_WALL, true, true);
    spr(art_.banner, 24, CONTACT - 118 + bob, 34, PAL_BANNER, false, true);
    spr(art_.banner, 296, CONTACT - 118 - bob, 34, PAL_BANNER, true, true);
    spr(art_.torch[torch], 40, CONTACT - 96, 26, PAL_FIRE, false, true);
    spr(art_.torch[1 - torch], 280, CONTACT - 96, 26, PAL_FIRE, false, true);

    spr(art_.moon, 292, 8, 26, PAL_MOON, false, true);
    spr(art_.star, 36, 10, 6, PAL_MOON, false, true);
    spr(art_.star, 78, 18, 5, PAL_MOON, false, true);
    spr(art_.star, 250, 14, 4, PAL_MOON, false, true);
    static const float tuft[][2] = {{48, 176}, {96, 198}, {150, 188}, {206, 170}, {250, 194}, {300, 210}};
    for (const auto& t : tuft) spr(art_.tuft, t[0], t[1], 12, PAL_GRASS, false, true);

    if (mode_ == Mode::Title) {
        hudC(24, "STOP ALL 8 RAMS", PAL_GOLD);
        hudC(25, "ARROWS SLIDE    DOWN OR X BRACE", PAL_HUD);
        hudC(26, "C OR Z STONE    ENTER START", PAL_DIM);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "ENTER RESUME", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_DIM);
    } else if (mode_ == Mode::Won) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(25, buf, PAL_GOLD);
        hudC(26, "ENTER AGAIN", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(25, through_ ? "THE WALL WAS NOT THERE" : "THE STONES GAVE WAY", PAL_ALERT);
        hudC(26, "ENTER AGAIN", PAL_HUD);
    } else {
        hud(1, 0, "GATE", PAL_HUD);
        for (int i = 0; i < GATE_MAX; i++) hud(6 + i, 0, gate_ > i ? "#" : "-", gate_ > i ? PAL_GOLD : PAL_DIM);
        char buf[24];
        std::snprintf(buf, sizeof buf, "RAMS %d/%d", stopped_, kRamCount);
        hud(13, 0, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(33, 0, buf, PAL_GOLD);
        bool iron = false;
        for (const Ram& r : rams_)
            if (r.alive && r.heavy) iron = true;
        if (braced_) hud(1, 1, "BRACED", PAL_GOLD);
        else if (iron && (int(anim_ * 5.0f) & 1)) hud(1, 1, "BRACE THE IRON", PAL_ALERT);
        else hud(1, 1, "OPEN", PAL_DIM);
        if (stone_) hud(30, 1, "FALLING", PAL_GOLD);
        else if (stoneCd_ > 0) hud(32, 1, "WAIT", PAL_DIM);
        else hud(31, 1, "STONE", PAL_GOLD);
        if (noteT_ > 0 && note_ && note_[0]) hudC(3, note_, notePal_);
        hudC(27, "ARROWS SLIDE   DOWN BRACE   C STONE", PAL_DIM);
    }
}

}  // namespace siege

#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace sally {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int HOLD = 180 * 60;
constexpr int SHOVE = 1;
constexpr int HEAVE = 2;
constexpr int YANK = 3;

constexpr float WARN = 0.45f;
constexpr float HIT_S = 0.62f;
constexpr float HIT_H = 0.88f;
constexpr float YANK_T = 0.58f;
constexpr float WEDGE_T = 2.1f;
constexpr float WEDGE_CD = 7.5f;
constexpr float CREEP = 0.0024f;
constexpr float SHOULDER = 0.05f;
constexpr float YANK_MISS = 0.36f;

constexpr float DOOR_L = 112.f;
constexpr float DOOR_R = 208.f;
constexpr float DOOR_T = 52.f;
constexpr float DOOR_B = 186.f;

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch hingePatch() {
    gs::FMPatch p;
    p.alg = 3;
    p.fb = 0.28f;
    p.op[0] = {1.f, 0.4f, 0.05f, 0.35f, 0.55f, 0.3f};
    p.op[1] = {1.5f, 0.18f, 0.07f, 0.28f, 0.4f, 0.22f};
    p.op[2] = {0.5f, 0.22f, 0.1f, 0.3f, 0.45f, 0.2f};
    p.op[3] = {2.01f, 0.1f, 0.04f, 0.2f, 0.25f, 0.16f};
    p.vol = 0.03f;
    p.drive = 0.1f;
    p.tone = 180.f;
    p.glide = 0.04f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    blip_ = 0.05f;
}

void Game::sparkAt(float x, float y) {
    for (Spark& d : sparks_) {
        if (d.life > 0.f) continue;
        d.x = x;
        d.y = y;
        d.vy = -22.f;
        d.life = 0.32f;
        return;
    }
}

void Game::tickSparks() {
    for (Spark& d : sparks_) {
        if (d.life <= 0.f) continue;
        d.life -= DT;
        d.y += d.vy * DT;
        d.vy += 50.f * DT;
    }
}

void Game::layBeats() {
    beatCount_ = 0;
    beatNext_ = 0;
    auto add = [&](double sec, int kind, int side) {
        if (beatCount_ >= int(sizeof beats_ / sizeof beats_[0])) return;
        beats_[beatCount_++] = Beat{int(std::lround(sec * 60.0)), kind, side};
    };
    double t = 6.0;
    int i = 0;
    while (t < 162.0) {
        int kind = (i % 6 == 5) ? HEAVE : SHOVE;
        int side = (i % 3 == 0) ? -1 : 1;
        add(t, kind, side);
        if (i % 5 == 4) add(t + 1.7, YANK, 0);
        t += 9.0 - 2.2 * (t / 180.0);
        i++;
    }
    add(166.0, SHOVE, 1);
    add(168.4, YANK, 0);
    add(170.2, HEAVE, -1);
    add(172.6, YANK, 0);
    add(174.8, SHOVE, 1);
    add(176.8, HEAVE, -1);
    std::sort(beats_, beats_ + beatCount_, [](const Beat& a, const Beat& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.kind < b.kind;
    });
}

void Game::hingeOn() {
    sys_->apu.setPatch(0, hingePatch());
    sys_->apu.keyOn(0, 74.f, 0.024f);
}

void Game::begin() {
    age_ = 0;
    gap_ = 0.f;
    arms_ = 1.f;
    slip_ = 0.f;
    wedgeT_ = 0.f;
    wedgeCd_ = 0.f;
    yankT_ = 0.f;
    chainCaught_ = false;
    yankOut_ = 0.f;
    warn_ = 0.f;
    shove_ = 0.f;
    pending_ = 0.f;
    side_ = 0;
    kind_ = 0;
    lean_ = 0;
    bracing_ = false;
    shouldering_ = false;
    shake_ = 0.f;
    recoil_ = 0.f;
    blip_ = 0.f;
    tick_ = 0.f;
    fanT_ = 0.f;
    fan_ = -1;
    won_ = false;
    over_ = false;
    gate_ = 10;
    for (Spark& d : sparks_) d.life = 0.f;
    layBeats();
    mode_ = Mode::Play;
    hingeOn();
    sys_->setLight(80, 50, 20);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    gap_ = 0.f;
    arms_ = 1.f;
    slip_ = 0.f;
    wedgeT_ = 0.f;
    wedgeCd_ = 0.f;
    yankT_ = 0.f;
    yankOut_ = 0.f;
    chainCaught_ = false;
    warn_ = 0.f;
    shove_ = 0.f;
    side_ = 0;
    kind_ = 0;
    lean_ = 0;
    bracing_ = false;
    shouldering_ = false;
    shake_ = 0.f;
    recoil_ = 0.f;
    fan_ = -1;
    gate_ = 16;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->setLight(20, 24, 48);
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    gate_ = 30;
    fan_ = held ? 0 : -1;
    fanT_ = 0.f;
    warn_ = 0.f;
    shove_ = 0.f;
    yankT_ = 0.f;
    bracing_ = false;
    shouldering_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    if (held) {
        gap_ = 0.f;
        shake_ = 0.25f;
        sys_->rumble(0.12f, 0.35f, 180);
        sys_->setLight(20, 110, 50);
    } else {
        gap_ = 1.f;
        shake_ = 1.0f;
        sys_->apu.noiseBurst(0.6f, 90.f, 0.4f);
        sys_->rumble(0.75f, 0.12f, 240);
        sys_->setLight(140, 18, 12);
    }
}

Game::Wish Game::wish() const {
    Wish w;
    if (bot_) {
        bool threat = warn_ > 0.f || shove_ > 0.f;
        if (yankT_ > 0.f && !chainCaught_) w.catchChain = true;
        if (threat && slip_ <= 0.f) w.lean = side_;
        else if (!threat && gap_ > 0.08f && slip_ <= 0.f && arms_ > 0.25f) w.shoulder = true;
        if (!threat && wedgeT_ <= 0.f && wedgeCd_ <= 0.f && gap_ > 0.18f) w.wedge = true;
        return w;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) w.lean = -1;
    else if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) w.lean = 1;
    w.catchChain = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z);
    w.wedge = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X);
    w.shoulder = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_DOWN) || p.accel > 0.45f;
    return w;
}

void Game::update() {
    while (beatNext_ < beatCount_ && beats_[beatNext_].frame <= age_) {
        const Beat& b = beats_[beatNext_++];
        if (b.kind == YANK) {
            yankT_ = YANK_T;
            chainCaught_ = false;
            blip(520.f);
        } else {
            kind_ = b.kind;
            side_ = b.side;
            warn_ = WARN;
            pending_ = (b.kind == HEAVE) ? HIT_H : HIT_S;
            blip(b.kind == HEAVE ? 110.f : 190.f);
        }
    }

    Wish w = wish();
    if (w.catchChain) {
        if (yankT_ > 0.f && !chainCaught_) {
            chainCaught_ = true;
            yankT_ = 0.f;
            gap_ = std::max(0.f, gap_ - 0.012f);
            blip(740.f);
            sys_->apu.noiseBurst(0.12f, 500.f, 0.04f);
            sparkAt(160.f, 64.f);
        } else if (!bot_) {
            blip(80.f);
        }
    }

    bool threat = warn_ > 0.f || shove_ > 0.f;
    if (w.wedge) {
        bool can = wedgeT_ <= 0.f && wedgeCd_ <= 0.f && gap_ < 0.55f && !threat;
        if (can) {
            wedgeT_ = WEDGE_T;
            wedgeCd_ = WEDGE_CD;
            gap_ = std::max(0.f, gap_ - 0.018f);
            blip(150.f);
            sys_->apu.noiseBurst(0.14f, 180.f, 0.05f);
            sparkAt(160.f, 178.f);
        } else if (!bot_ && wedgeT_ <= 0.f) {
            blip(64.f);
        }
    }

    bool bracing = threat && w.lean == side_ && side_ != 0 && arms_ > 0.02f && slip_ <= 0.f;
    bool shouldering = w.shoulder && !threat && arms_ > 0.02f && slip_ <= 0.f;
    if (slip_ > 0.f) {
        slip_ -= DT;
        if (slip_ < 0.f) slip_ = 0.f;
        arms_ = std::min(1.f, arms_ + 0.16f * DT);
        bracing = false;
        shouldering = false;
    } else {
        float drain = (bracing ? 0.07f : 0.f) + (shouldering ? 0.035f : 0.f);
        if (drain > 0.f) {
            arms_ -= drain * DT;
            if (arms_ <= 0.f) {
                arms_ = 0.f;
                slip_ = 0.42f;
                bracing = false;
                shouldering = false;
                blip(72.f);
            }
        } else {
            arms_ = std::min(1.f, arms_ + 0.2f * DT);
        }
    }
    bracing_ = bracing;
    shouldering_ = shouldering;
    lean_ = w.lean;

    float push = CREEP;
    if (shove_ > 0.f) {
        float base = bracing_ ? ((kind_ == HEAVE) ? 0.05f : 0.032f) : ((kind_ == HEAVE) ? 0.7f : 0.5f);
        if (lean_ != 0 && lean_ != side_) base += 0.1f;
        if (wedgeT_ > 0.f) base *= 0.25f;
        if (yankOut_ > 0.f) base *= 1.35f;
        push += base;
    }
    if (shouldering_) push -= SHOULDER;
    gap_ += push * DT;

    if (wedgeT_ > 0.f) wedgeT_ = std::max(0.f, wedgeT_ - DT);
    if (wedgeCd_ > 0.f) wedgeCd_ = std::max(0.f, wedgeCd_ - DT);
    if (yankT_ > 0.f) {
        yankT_ -= DT;
        if (yankT_ <= 0.f) {
            yankT_ = 0.f;
            if (!chainCaught_) {
                gap_ += YANK_MISS;
                yankOut_ = 1.3f;
                shake_ = std::max(shake_, 0.8f);
                blip(90.f);
                sys_->apu.noiseBurst(0.32f, 140.f, 0.12f);
            }
        }
    }
    if (yankOut_ > 0.f) yankOut_ = std::max(0.f, yankOut_ - DT);
    if (warn_ > 0.f) {
        warn_ -= DT;
        if (warn_ <= 0.f) {
            warn_ = 0.f;
            shove_ = pending_;
            recoil_ = 1.f;
            shake_ = std::max(shake_, bracing_ ? 0.22f : 0.7f);
            if (!bracing_) gap_ += (kind_ == HEAVE) ? 0.05f : 0.03f;
            sys_->apu.noiseBurst(bracing_ ? 0.16f : 0.42f, bracing_ ? 160.f : 90.f, 0.1f);
            sys_->rumble(bracing_ ? 0.18f : 0.6f, 0.1f, 50);
            sparkAt(side_ < 0 ? DOOR_L + 18.f : DOOR_R - 18.f, 130.f);
        }
    } else if (shove_ > 0.f) {
        shove_ = std::max(0.f, shove_ - DT);
    }

    if (gap_ < 0.f) gap_ = 0.f;
    if (gap_ >= 1.f) {
        finish(false);
        return;
    }
    if ((bracing_ || shouldering_) && (age_ % 16 == 0)) sparkAt(lean_ < 0 ? 128.f : 192.f, 170.f);

    float hz = 58.f + gap_ * 140.f + (shove_ > 0.f ? 22.f : 0.f);
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, 0.014f + gap_ * 0.05f + (bracing_ ? 0.016f : 0.f));

    age_++;
    if (age_ >= HOLD) {
        finish(true);
        return;
    }
    if (age_ % 60 == 0) {
        bool late = age_ >= HOLD - 600;
        sys_->apu.tone(2, late ? 620.f : 220.f, late ? 0.04f : 0.022f);
        tick_ = 0.04f;
    }
}

void Game::fanfare() {
    if (fan_ < 0) return;
    fanT_ += DT;
    if (fan_ < 4 && fanT_ >= 0.2f) {
        static const float notes[] = {262.f, 330.f, 392.f, 523.f};
        sys_->apu.tone(1, notes[fan_], 0.06f);
        fan_++;
        fanT_ = 0.f;
    } else if (fan_ >= 4 && fanT_ > 0.45f) {
        sys_->apu.tone(1, 0, 0);
        fan_ = -1;
    }
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= DT;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (tick_ > 0.f) {
        tick_ -= DT;
        if (tick_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.B.resize(64, 32);
    sys.vdp.B.clear();
    for (int y = 0; y < 32; y++)
        for (int x = 0; x < 64; x++) sys.vdp.B.set(x, y, gs::entry(1, PAL_STONE));
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 3));
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    gate_ = 18;
    gap_ = 0.f;
    arms_ = 1.f;
    sys.setLight(20, 24, 48);
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.04f);
    if (recoil_ > 0.f) recoil_ = std::max(0.f, recoil_ - 0.07f);

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) ||
                 pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (mode_ == Mode::Title) {
        if (gate_ == 0 && start) begin();
    } else if (mode_ == Mode::Play) {
        if (gate_ == 0 && pad.pressed(gs::BTN_START) && !bot_) {
            mode_ = Mode::Pause;
            sys.apu.keyOff(0);
            sys.apu.tone(0, 0, 0);
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            gate_ = 8;
            hingeOn();
        }
    } else {
        if (mode_ == Mode::Won) fanfare();
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) toTitle();
    }
    tickSparks();
    serviceAudio();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    float width = 0.f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) {
            x += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal);
        x += gw;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    stamp(m, cx, cy, h * float(m.w) / float(m.h), h, pal, flip, fog);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip, int fog) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(20, 110, 50);
    else if (mode_ == Mode::Lost) sys_->setLight(140, 18, 12);
    else if (mode_ == Mode::Title) sys_->setLight(20, 24, 48);
    else if (yankT_ > 0.f || gap_ > 0.55f) sys_->setLight(130, 30, 16);
    else if (bracing_ || shouldering_) sys_->setLight(120, 70, 18);
    else sys_->setLight(60, 40, 24);
}

void Game::scene(float shx) {
    gs::VDP& v = sys_->vdp;
    float danger = (mode_ == Mode::Lost) ? 1.f : std::clamp(gap_, 0.f, 1.f) * 0.45f;
    uint16_t sky = mixC(gs::rgb4(1, 1, 4), gs::rgb4(5, 1, 1), danger);
    uint16_t mid = mixC(gs::rgb4(2, 2, 5), gs::rgb4(6, 2, 2), danger);
    uint16_t low = mixC(gs::rgb4(2, 2, 3), gs::rgb4(5, 2, 1), danger * 0.7f);
    if (mode_ == Mode::Won) {
        sky = mixC(sky, gs::rgb4(1, 4, 3), 0.5f);
        mid = mixC(mid, gs::rgb4(2, 6, 4), 0.35f);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 40) v.lineBackdrop[y] = sky;
        else if (y < 148) v.lineBackdrop[y] = mid;
        else v.lineBackdrop[y] = low;
    }
    v.B.enabled = true;
    v.B.scroll(int(shx * 0.4f), 2);
    v.setFogColor(gs::rgb4(1, 1, 2));

    for (int y = 170; y < gs::SCREEN_H; y++) {
        float t = (float(y) - 170.f) / 54.f;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx * 0.2f;
        rd.hw = 64.f + t * 96.f;
        rd.v = 4.f + t * 22.f;
        rd.pal = PAL_ROAD;
        rd.style = 1;
        rd.band = (y / 8) & 1;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        v.lineFog[y] = 3;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float shx = 0.f, shy = 0.f;
    if (shake_ > 0.f) {
        shx = std::sin(float(age_ + sys_->frame) * 1.9f) * shake_ * 4.f;
        shy = std::cos(float(age_ + sys_->frame) * 2.3f) * shake_ * 1.8f;
    }
    scene(shx);
    lamp();

    const bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    float show = gap_;
    if (mode_ == Mode::Title) show = 0.03f + 0.015f * std::sin(float(sys_->frame) * 0.05f);
    else if (mode_ == Mode::Won) show = 0.f;
    else if (mode_ == Mode::Lost) show = 1.f;
    show = std::clamp(show, 0.f, 1.f);

    auto X = [&](float x) { return x + shx; };
    float open = (DOOR_R - DOOR_L) * show * 0.55f;
    float leafL = DOOR_L + open;
    float leafR = DOOR_R + recoil_ * (side_ > 0 ? 3.f : -2.f);
    if (leafR < leafL + 12.f) leafR = leafL + 12.f;
    float leafCx = (leafL + leafR) * 0.5f;
    float leafW = leafR - leafL;
    float leafCy = (DOOR_T + DOOR_B) * 0.5f + shy;
    float leafH = DOOR_B - DOOR_T;
    int step = (sys_->frame / 9) & 1;
    bool flick = ((sys_->frame / 5) & 1) != 0;

    spr(art_.moon, X(48.f), 22.f, 16.f, PAL_TEXT);
    spr(art_.banner, X(86.f), 36.f, 28.f, PAL_BANNER);
    spr(art_.banner, X(234.f), 40.f, 24.f, PAL_BANNER, true);

    for (Spark& d : sparks_) {
        if (d.life > 0.04f) spr(art_.spark, X(d.x), d.y, 6.f, PAL_TORCH);
    }

    spr(art_.helm[shouldering_ ? 1 : 0], X(lean_ < 0 ? 136.f : 184.f), 164.f + shy, bracing_ ? 58.f : 52.f, PAL_MAIL,
        lean_ > 0);
    if (wedgeT_ > 0.f) spr(art_.wedge, X(160.f), 182.f, 10.f, PAL_WOOD);

    float rise = show * 36.f;
    for (int i = 0; i < 7; i++) spr(art_.bar, X(124.f + float(i) * 12.f), DOOR_T + 28.f - rise + shy, 70.f, PAL_IRON);
    stamp(art_.leaf, X(leafCx), leafCy, leafW, leafH, PAL_IRON);
    if (yankT_ > 0.f) spr(art_.chain, X(160.f), DOOR_T + 14.f + (1.f - yankT_ / YANK_T) * 20.f, 32.f, PAL_WOOD);
    if (yankOut_ > 0.f) spr(art_.chain, X(172.f), 86.f, 24.f, PAL_ALERT);

    bool threat = live && (warn_ > 0.f || shove_ > 0.f);
    if (threat || mode_ == Mode::Lost) {
        float px = side_ < 0 ? DOOR_L + 14.f : DOOR_R - 18.f;
        if (mode_ == Mode::Lost) px = 156.f;
        spr(art_.ram[step], X(px), 146.f + shy, 64.f, PAL_MAIL, side_ > 0, mode_ == Mode::Lost ? 0 : 5);
    }

    spr(art_.pier, X(DOOR_L - 10.f), leafCy, leafH + 10.f, PAL_STONE);
    spr(art_.pier, X(DOOR_R + 10.f), leafCy, leafH + 10.f, PAL_STONE);
    stamp(art_.arch, X(160.f), DOOR_T - 4.f + shy * 0.15f, 140.f, 22.f, PAL_STONE);
    spr(art_.torch, X(96.f), 70.f, flick ? 30.f : 26.f, flick ? PAL_TORCH : PAL_WOOD);
    spr(art_.torch, X(224.f), 74.f, flick ? 26.f : 30.f, flick ? PAL_WOOD : PAL_TORCH);
    spr(art_.grate, X(160.f), 202.f, 12.f, PAL_IRON);

    if (mode_ == Mode::Title) {
        text("S3 SALLY DOOR", 160.f, 16.f, 0.55f, PAL_TORCH);
        text("YOU HAVE THE SALLY", 160.f, 36.f, 0.34f, PAL_TEXT);
        text("HOLD THE DOOR", 160.f, 52.f, 0.4f, PAL_BANNER);
        hudC(24, "LEAN  CHAIN A  BAR B  SHOULDER C", PAL_TEXT);
        hudC(26, "START", PAL_TORCH);
    } else if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160.f, 16.f, 0.52f, PAL_TORCH);
        text("THREE MINUTES", 160.f, 36.f, 0.44f, PAL_GOOD);
        text("THE SALLY IS YOURS", 160.f, 54.f, 0.32f, PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        text("THE DOOR OPENED", 160.f, 16.f, 0.46f, PAL_ALERT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 16.f, 0.64f, PAL_TORCH);
    } else {
        int remain = std::max(0, HOLD - age_);
        int sec = (remain + 59) / 60;
        char clock[16];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        text(clock, 160.f, 14.f, 0.52f, remain < 600 ? PAL_ALERT : PAL_TORCH);
        int bars = int(std::lround((1.f - gap_) * 16.f));
        char seam[20];
        for (int i = 0; i < 16; i++) seam[i] = (i < bars) ? '#' : '.';
        seam[16] = 0;
        hud(12, 26, seam, gap_ > 0.55f ? PAL_ALERT : PAL_GOOD);
        if (yankT_ > 0.f && !chainCaught_) hudC(3, "CHAIN", PAL_ALERT);
        else if (warn_ > 0.f) hudC(3, side_ < 0 ? "LEFT" : "RIGHT", PAL_TORCH);
        else if (slip_ > 0.f) hudC(3, "SLIP", PAL_ALERT);
    }
}

}  // namespace sally

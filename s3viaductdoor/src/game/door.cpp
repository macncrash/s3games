#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace viaduct {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int HOLD = 180 * 60;
constexpr int GUST = 1;
constexpr int GALE = 2;
constexpr int HITCH = 3;

constexpr float WARN = 0.42f;
constexpr float HIT_G = 0.70f;
constexpr float HIT_A = 0.96f;
constexpr float HITCH_T = 0.62f;
constexpr float CHOCK_T = 2.3f;
constexpr float CHOCK_CD = 8.5f;
constexpr float CREEP = 0.0028f;
constexpr float SET = 0.055f;
constexpr float HITCH_MISS = 0.40f;

constexpr float GATE_X = 160.f;
constexpr float GATE_W = 72.f;
constexpr float GATE_TOP = 52.f;
constexpr float GATE_BOT = 176.f;

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch hingePatch() {
    gs::FMPatch p;
    p.alg = 2;
    p.fb = 0.22f;
    p.op[0] = {1.f, 0.4f, 0.05f, 0.45f, 0.55f, 0.3f};
    p.op[1] = {1.5f, 0.18f, 0.08f, 0.28f, 0.35f, 0.22f};
    p.op[2] = {0.5f, 0.25f, 0.1f, 0.3f, 0.4f, 0.2f};
    p.op[3] = {2.01f, 0.1f, 0.04f, 0.2f, 0.25f, 0.16f};
    p.vol = 0.03f;
    p.drive = 0.08f;
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

void Game::puff(float x, float y) {
    for (Mote& d : motes_) {
        if (d.life > 0.f) continue;
        d.x = x;
        d.y = y;
        d.vy = -22.f;
        d.life = 0.32f;
        return;
    }
}

void Game::tickMotes() {
    float wind = float(sys_->frame) * 0.4f;
    for (int i = 0; i < 24; i++) {
        Mote& d = motes_[i];
        if (d.life > 0.f) {
            d.life -= DT;
            d.y += d.vy * DT;
            d.vy += 36.f * DT;
            d.x += (side_ == 0 ? 8.f : float(side_) * 30.f) * DT;
            continue;
        }
        d.x = std::fmod(wind + float(i) * 47.f, 340.f) - 10.f;
        d.y += (40.f + float(i % 5) * 8.f) * DT;
        if (d.y > gs::SCREEN_H + 4.f) d.y = -6.f;
    }
}

void Game::layBeats() {
    beatCount_ = 0;
    beatNext_ = 0;
    auto add = [&](double sec, int kind, int side) {
        if (beatCount_ >= int(sizeof beats_ / sizeof beats_[0])) return;
        beats_[beatCount_++] = Beat{int(std::lround(sec * 60.0)), kind, side};
    };
    double t = 5.5;
    int i = 0;
    while (t < 164.0) {
        int kind = (i % 5 == 4) ? GALE : GUST;
        int side = (i & 1) ? 1 : -1;
        add(t, kind, side);
        if (i % 4 == 3) add(t + 1.55, HITCH, 0);
        t += 8.4 - 2.6 * (t / 180.0);
        i++;
    }
    add(168.2, GUST, -1);
    add(170.0, HITCH, 0);
    add(171.6, GALE, 1);
    add(173.6, HITCH, 0);
    add(175.2, GUST, -1);
    add(177.0, GALE, 1);
    std::sort(beats_, beats_ + beatCount_, [](const Beat& a, const Beat& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.kind < b.kind;
    });
}

void Game::hingeOn() {
    sys_->apu.setPatch(0, hingePatch());
    sys_->apu.keyOn(0, 78.f, 0.026f);
}

void Game::begin() {
    age_ = 0;
    gap_ = 0.f;
    grip_ = 1.f;
    slip_ = 0.f;
    chockT_ = 0.f;
    chockCd_ = 0.f;
    hitchT_ = 0.f;
    hitchCaught_ = false;
    hitchOut_ = 0.f;
    warn_ = 0.f;
    gust_ = 0.f;
    pending_ = 0.f;
    side_ = 0;
    kind_ = 0;
    lean_ = 0;
    bracing_ = false;
    setting_ = false;
    shake_ = 0.f;
    recoil_ = 0.f;
    blip_ = 0.f;
    tick_ = 0.f;
    fanT_ = 0.f;
    fan_ = -1;
    won_ = false;
    over_ = false;
    gate_ = 10;
    for (Mote& d : motes_) {
        d.life = 0.f;
        d.y = -8.f;
    }
    layBeats();
    mode_ = Mode::Play;
    hingeOn();
    sys_->setLight(70, 55, 30);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    gap_ = 0.f;
    grip_ = 1.f;
    slip_ = 0.f;
    chockT_ = hitchT_ = hitchOut_ = 0.f;
    hitchCaught_ = false;
    warn_ = gust_ = 0.f;
    side_ = kind_ = lean_ = 0;
    bracing_ = setting_ = false;
    shake_ = recoil_ = 0.f;
    fan_ = -1;
    gate_ = 16;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->setLight(30, 36, 60);
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    gate_ = 30;
    fan_ = held ? 0 : -1;
    fanT_ = 0.f;
    warn_ = gust_ = hitchT_ = 0.f;
    bracing_ = setting_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    if (held) {
        gap_ = 0.f;
        shake_ = 0.3f;
        sys_->rumble(0.12f, 0.35f, 180);
        sys_->setLight(24, 110, 70);
    } else {
        gap_ = 1.f;
        shake_ = 1.1f;
        sys_->apu.noiseBurst(0.6f, 90.f, 0.4f);
        sys_->rumble(0.75f, 0.12f, 240);
        sys_->setLight(140, 28, 16);
    }
}

Game::Wish Game::wish() const {
    Wish w;
    if (bot_) {
        bool threat = warn_ > 0.f || gust_ > 0.f;
        if (hitchT_ > 0.f && !hitchCaught_) w.catchHitch = true;
        if (threat && slip_ <= 0.f) w.lean = side_;
        else if (!threat && gap_ > 0.10f && slip_ <= 0.f && grip_ > 0.2f) w.set = true;
        if (!threat && chockT_ <= 0.f && chockCd_ <= 0.f && gap_ > 0.22f) w.chock = true;
        return w;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) w.lean = -1;
    else if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) w.lean = 1;
    w.catchHitch = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z);
    w.chock = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X);
    w.set = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_DOWN) || p.accel > 0.45f;
    return w;
}

void Game::update() {
    while (beatNext_ < beatCount_ && beats_[beatNext_].frame <= age_) {
        const Beat& b = beats_[beatNext_++];
        if (b.kind == HITCH) {
            hitchT_ = HITCH_T;
            hitchCaught_ = false;
            blip(520.f);
        } else {
            kind_ = b.kind;
            side_ = b.side;
            warn_ = WARN;
            pending_ = (b.kind == GALE) ? HIT_A : HIT_G;
            blip(b.kind == GALE ? 110.f : 210.f);
        }
    }

    Wish w = wish();
    if (w.catchHitch) {
        if (hitchT_ > 0.f && !hitchCaught_) {
            hitchCaught_ = true;
            hitchT_ = 0.f;
            gap_ = std::max(0.f, gap_ - 0.015f);
            blip(740.f);
            sys_->apu.noiseBurst(0.12f, 520.f, 0.04f);
            puff(GATE_X, 64.f);
        } else if (!bot_) {
            blip(80.f);
        }
    }

    bool threat = warn_ > 0.f || gust_ > 0.f;
    if (w.chock) {
        bool can = chockT_ <= 0.f && chockCd_ <= 0.f && gap_ < 0.5f && !threat;
        if (can) {
            chockT_ = CHOCK_T;
            chockCd_ = CHOCK_CD;
            gap_ = std::max(0.f, gap_ - 0.02f);
            blip(160.f);
            sys_->apu.noiseBurst(0.14f, 180.f, 0.05f);
            puff(GATE_X, 168.f);
        } else if (!bot_ && chockT_ <= 0.f) {
            blip(64.f);
        }
    }

    bool bracing = threat && w.lean == side_ && side_ != 0 && grip_ > 0.02f && slip_ <= 0.f;
    bool setting = w.set && !threat && grip_ > 0.02f && slip_ <= 0.f;
    if (slip_ > 0.f) {
        slip_ -= DT;
        if (slip_ < 0.f) slip_ = 0.f;
        grip_ = std::min(1.f, grip_ + 0.14f * DT);
        bracing = setting = false;
    } else {
        float drain = (bracing ? 0.08f : 0.f) + (setting ? 0.04f : 0.f);
        if (drain > 0.f) {
            grip_ -= drain * DT;
            if (grip_ <= 0.f) {
                grip_ = 0.f;
                slip_ = 0.48f;
                bracing = setting = false;
                blip(72.f);
            }
        } else {
            grip_ = std::min(1.f, grip_ + 0.18f * DT);
        }
    }
    bracing_ = bracing;
    setting_ = setting;
    lean_ = w.lean;

    float push = CREEP;
    if (gust_ > 0.f) {
        float base = bracing_ ? ((kind_ == GALE) ? 0.06f : 0.04f) : ((kind_ == GALE) ? 0.78f : 0.58f);
        if (lean_ != 0 && lean_ != side_) base += 0.12f;
        if (chockT_ > 0.f) base *= 0.22f;
        if (hitchOut_ > 0.f) base *= 1.45f;
        push += base;
    }
    if (setting_) push -= SET;
    gap_ += push * DT;

    if (chockT_ > 0.f) chockT_ = std::max(0.f, chockT_ - DT);
    if (chockCd_ > 0.f) chockCd_ = std::max(0.f, chockCd_ - DT);
    if (hitchT_ > 0.f) {
        hitchT_ -= DT;
        if (hitchT_ <= 0.f) {
            hitchT_ = 0.f;
            if (!hitchCaught_) {
                gap_ += HITCH_MISS;
                hitchOut_ = 1.4f;
                shake_ = std::max(shake_, 0.85f);
                blip(96.f);
                sys_->apu.noiseBurst(0.32f, 140.f, 0.12f);
            }
        }
    }
    if (hitchOut_ > 0.f) hitchOut_ = std::max(0.f, hitchOut_ - DT);
    if (warn_ > 0.f) {
        warn_ -= DT;
        if (warn_ <= 0.f) {
            warn_ = 0.f;
            gust_ = pending_;
            recoil_ = 1.f;
            shake_ = std::max(shake_, bracing_ ? 0.22f : 0.7f);
            if (!bracing_) gap_ += (kind_ == GALE) ? 0.06f : 0.035f;
            sys_->apu.noiseBurst(bracing_ ? 0.16f : 0.42f, bracing_ ? 240.f : 100.f, 0.1f);
            sys_->rumble(bracing_ ? 0.18f : 0.55f, 0.1f, 50);
            puff(side_ < 0 ? 96.f : 224.f, 120.f);
        }
    } else if (gust_ > 0.f) {
        gust_ = std::max(0.f, gust_ - DT);
    }

    if (gap_ < 0.f) gap_ = 0.f;
    if (gap_ >= 1.f) {
        finish(false);
        return;
    }
    if ((bracing_ || setting_) && (age_ % 14 == 0)) puff(lean_ < 0 ? 124.f : 196.f, 160.f);

    float hz = 62.f + gap_ * 160.f + (gust_ > 0.f ? 24.f : 0.f);
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, 0.014f + gap_ * 0.05f + (bracing_ ? 0.016f : 0.f));

    age_++;
    if (age_ >= HOLD) {
        finish(true);
        return;
    }
    if (age_ % 60 == 0) {
        bool late = age_ >= HOLD - 600;
        sys_->apu.tone(2, late ? 620.f : 246.f, late ? 0.04f : 0.024f);
        tick_ = 0.04f;
    }
}

void Game::fanfare() {
    if (fan_ < 0) return;
    fanT_ += DT;
    if (fan_ < 4 && fanT_ >= 0.18f) {
        static const float notes[] = {311.f, 392.f, 466.f, 622.f};
        sys_->apu.tone(1, notes[fan_], 0.06f);
        fan_++;
        fanT_ = 0.f;
    } else if (fan_ >= 4 && fanT_ > 0.4f) {
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
        for (int x = 0; x < 64; x++) sys.vdp.B.set(x, y, gs::entry(art_.course, PAL_STONE));
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 4));
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    gate_ = 18;
    gap_ = 0.f;
    grip_ = 1.f;
    sys.setLight(30, 36, 60);
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
    } else if (gate_ == 0 && pad.pressed(gs::BTN_START)) {
        toTitle();
    }
    if (mode_ == Mode::Won) fanfare();
    tickMotes();
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

void Game::spanLight() {
    if (mode_ == Mode::Won) sys_->setLight(24, 110, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(140, 28, 16);
    else if (mode_ == Mode::Title) sys_->setLight(30, 36, 60);
    else if (hitchT_ > 0.f || gap_ > 0.55f) sys_->setLight(130, 36, 18);
    else if (bracing_ || setting_) sys_->setLight(120, 80, 28);
    else sys_->setLight(60, 50, 36);
}

void Game::scene(float shx) {
    gs::VDP& v = sys_->vdp;
    float danger = (mode_ == Mode::Lost) ? 1.f : std::clamp(gap_, 0.f, 1.f) * 0.55f;
    uint16_t sky = mixC(gs::rgb4(2, 2, 5), gs::rgb4(7, 3, 2), danger);
    uint16_t gorge = mixC(gs::rgb4(1, 2, 4), gs::rgb4(5, 2, 2), danger);
    if (mode_ == Mode::Won) sky = mixC(sky, gs::rgb4(2, 6, 5), 0.5f);
    int horizon = 108;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = uint8_t(y < 40 ? 1 : 0);
        v.lineBackdrop[y] = y < horizon ? sky : gorge;
    }
    v.B.enabled = true;
    v.B.scroll(int(shx * 0.4f), 0);
    v.setFogColor(gs::rgb4(3, 3, 5));
    float scroll = float(age_) * 0.35f;
    for (int y = horizon; y < gs::SCREEN_H; y++) {
        float t = float(y - horizon) / float(gs::SCREEN_H - horizon);
        float denom = 0.08f + t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx * 0.25f + (side_ * gust_) * 6.f * (1.f - t);
        rd.hw = 18.f / denom;
        if (rd.hw > 150.f) rd.hw = 150.f;
        rd.v = scroll + 40.f / denom;
        rd.pal = PAL_DECK;
        rd.style = 1;
        rd.band = (y / 8) & 1;
        rd.left = gs::GROUND_DROP;
        rd.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int((1.f - t) * 8.f), 0, 8));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float shx = 0.f, shy = 0.f;
    if (shake_ > 0.f) {
        shx = std::sin(float(age_ + sys_->frame) * 1.6f) * shake_ * 4.f;
        shy = std::cos(float(age_ + sys_->frame) * 2.0f) * shake_ * 2.f;
    }
    scene(shx);
    spanLight();

    float show = gap_;
    if (mode_ == Mode::Title) show = 0.03f + 0.015f * std::sin(float(sys_->frame) * 0.05f);
    else if (mode_ == Mode::Won) show = 0.f;
    else if (mode_ == Mode::Lost) show = 1.f;
    show = std::clamp(show, 0.f, 1.f);

    auto X = [&](float x) { return x + shx; };
    float lift = show * 78.f;
    float gateCy = (GATE_TOP + GATE_BOT) * 0.5f - lift + shy;
    float gateH = GATE_BOT - GATE_TOP;
    int step = (sys_->frame / 8) & 1;
    bool lampOn = ((sys_->frame / 10) & 1) != 0;

    for (const Mote& d : motes_) {
        if (d.y < -4.f || d.y > gs::SCREEN_H) continue;
        spr(d.life > 0.05f ? art_.rivet : art_.mote, X(d.x), d.y, d.life > 0.05f ? 6.f : 7.f,
            d.life > 0.05f ? PAL_LAMP : PAL_SPARK, false, d.life > 0.05f ? 0 : 8);
    }

    spr(art_.pier, X(78.f), 130.f + shy * 0.2f, 150.f, PAL_STONE);
    spr(art_.pier, X(242.f), 130.f + shy * 0.2f, 150.f, PAL_STONE, true);
    stamp(art_.crown, X(160.f), 46.f + shy * 0.15f, 168.f, 22.f, PAL_STONE);

    float wardenX = lean_ < 0 ? 118.f : (lean_ > 0 ? 202.f : 186.f);
    spr(art_.warden[setting_ ? 1 : 0], X(wardenX), 158.f + shy, bracing_ ? 58.f : 52.f, PAL_COAT, lean_ > 0);
    if (chockT_ > 0.f) spr(art_.chock, X(160.f), 172.f, 14.f, PAL_IRON);

    stamp(art_.slab, X(GATE_X + recoil_ * float(side_)), gateCy, GATE_W, gateH, PAL_IRON);
    if (hitchT_ > 0.f)
        spr(art_.hitch, X(GATE_X), GATE_TOP + 8.f + (1.f - hitchT_ / HITCH_T) * 28.f, 34.f, PAL_LAMP);
    if (hitchOut_ > 0.f) spr(art_.hitch, X(174.f), 70.f, 26.f, PAL_ALERT);

    bool threat = (mode_ == Mode::Play || mode_ == Mode::Pause) && (warn_ > 0.f || gust_ > 0.f);
    if (threat || mode_ == Mode::Lost) {
        float px = side_ < 0 ? 108.f : 212.f;
        if (mode_ == Mode::Lost) px = 160.f;
        spr(art_.gust[step], X(px), 132.f + shy, 48.f, PAL_COAT, side_ > 0, 3);
    }

    float flap = std::sin(float(sys_->frame) * 0.18f) * 4.f;
    spr(art_.flag, X(54.f + flap), 78.f, 30.f, (side_ < 0 && threat) ? PAL_ALERT : PAL_FLAG, side_ > 0);
    spr(art_.flag, X(266.f - flap), 86.f, 26.f, (side_ > 0 && threat) ? PAL_ALERT : PAL_FLAG, true);
    spr(art_.lamp, X(96.f), 36.f, 22.f, lampOn ? PAL_LAMP : PAL_IRON);
    spr(art_.lamp, X(224.f), 36.f, 22.f, lampOn ? PAL_LAMP : PAL_IRON);

    char clock[16];
    if (mode_ == Mode::Title) {
        text("S3 VIADUCT DOOR", 160.f, 16.f, 0.5f, PAL_LAMP);
        text("HOLD THE GATE", 160.f, 36.f, 0.4f, PAL_TEXT);
        text("THREE MINUTES", 160.f, 52.f, 0.36f, PAL_FLAG);
        hudC(24, "LEAN  HITCH A  CHOCK B  SET C", PAL_TEXT);
        hudC(26, "START", PAL_LAMP);
    } else if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160.f, 16.f, 0.52f, PAL_LAMP);
        text("THREE MINUTES", 160.f, 38.f, 0.42f, PAL_GOOD);
        text("THE SPAN IS YOURS", 160.f, 56.f, 0.32f, PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        text("THE DOOR OPENED", 160.f, 16.f, 0.46f, PAL_ALERT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 16.f, 0.64f, PAL_LAMP);
    } else {
        int remain = std::max(0, HOLD - age_);
        int sec = (remain + 59) / 60;
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        hud(1, 1, clock, remain < 600 ? PAL_ALERT : PAL_TEXT);
        hud(32, 1, bracing_ ? "BRACE" : (setting_ ? "SET" : "HOLD"), PAL_LAMP);
        int bars = int(std::lround(grip_ * 8.f));
        char grip[16] = "GRIP ";
        for (int i = 0; i < 8; i++) grip[5 + i] = i < bars ? '#' : '.';
        grip[13] = 0;
        hud(1, 26, grip, slip_ > 0.f ? PAL_ALERT : PAL_TEXT);
        if (warn_ > 0.f) hudC(3, side_ < 0 ? "WIND  LEFT" : "WIND  RIGHT", PAL_ALERT);
        if (hitchT_ > 0.f) hudC(4, "HITCH", PAL_LAMP);
    }
}

}  // namespace viaduct

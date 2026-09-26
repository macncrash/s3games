#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace yarddoor {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int HOLD = 180 * 60;
constexpr int SHOVE = 1;
constexpr int LOG = 2;
constexpr int BAR = 3;

constexpr float TELE = 0.40f;
constexpr float HIT_S = 0.82f;
constexpr float HIT_L = 1.08f;
constexpr float BAR_T = 0.70f;
constexpr float CHOCK_T = 2.2f;
constexpr float CHOCK_CD = 9.f;
constexpr float CREEP = 0.0032f;
constexpr float SHOULDER = 0.045f;
constexpr float BAR_MISS = 0.46f;

constexpr float HORIZON = 84.f;
constexpr float ZNEAR = 2.0f;
constexpr float DOOR_TOP = 76.f;
constexpr float DOOR_H = 114.f;
constexpr float DOOR_L0 = 78.f;
constexpr float DOOR_W0 = 164.f;

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch creakPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.4f;
    p.op[0] = {1.f, 0.6f, 0.05f, 0.36f, 0.72f, 0.4f};
    p.op[1] = {1.98f, 0.26f, 0.08f, 0.42f, 0.42f, 0.32f};
    p.op[2] = {0.5f, 0.34f, 0.1f, 0.4f, 0.62f, 0.3f};
    p.op[3] = {1.48f, 0.16f, 0.05f, 0.28f, 0.36f, 0.26f};
    p.vol = 0.04f;
    p.drive = 0.16f;
    p.tone = 360.f;
    p.glide = 0.02f;
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
    sys_->apu.tone(0, freq, 0.055f);
    blip_ = 0.05f;
}

void Game::burst(float x, float y) {
    for (int i = 0; i < 12; i++) {
        Spark& s = sparks_[i];
        if (s.life > 0.f) continue;
        float ang = float((age_ + i * 5) % 13) * 0.48f;
        s.x = x;
        s.y = y;
        s.vx = std::sin(ang) * 42.f;
        s.vy = -30.f - float((age_ + i) % 5) * 6.f;
        s.life = 0.34f;
        return;
    }
}

void Game::tickSparks() {
    for (Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        s.life -= DT;
        s.x += s.vx * DT;
        s.y += s.vy * DT;
        s.vy += 64.f * DT;
    }
}

void Game::schedule() {
    markCount_ = 0;
    markNext_ = 0;
    auto add = [&](double sec, int kind, int side) {
        if (markCount_ >= int(sizeof marks_ / sizeof marks_[0])) return;
        marks_[markCount_++] = Mark{int(std::lround(sec * 60.0)), kind, side};
    };
    double t = 4.5;
    int i = 0;
    while (t < 166.0) {
        int kind = (i % 4 == 3) ? LOG : SHOVE;
        int side = (i % 2 == 0) ? -1 : 1;
        add(t, kind, side);
        if (i % 3 == 2) add(t + 1.70, BAR, 0);
        t += 7.8 - 3.0 * (t / 180.0);
        i++;
    }
    add(169.5, SHOVE, -1);
    add(171.4, BAR, 0);
    add(173.8, LOG, 1);
    add(176.0, BAR, 0);
    add(178.1, SHOVE, 1);
    std::sort(marks_, marks_ + markCount_, [](const Mark& a, const Mark& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.kind < b.kind;
    });
}

void Game::creakOn() {
    sys_->apu.setPatch(0, creakPatch());
    sys_->apu.keyOn(0, 86.f, 0.03f);
}

void Game::begin() {
    age_ = 0;
    slide_ = 0.f;
    arms_ = 1.f;
    slip_ = 0.f;
    chockT_ = 0.f;
    chockCd_ = 0.f;
    barT_ = 0.f;
    barCaught_ = false;
    barOut_ = 0.f;
    tele_ = 0.f;
    hit_ = 0.f;
    pendingHit_ = 0.f;
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
    gate_ = 12;
    for (Spark& s : sparks_) s.life = 0.f;
    schedule();
    mode_ = Mode::Play;
    creakOn();
    sys_->setLight(140, 86, 28);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    slide_ = 0.f;
    arms_ = 1.f;
    slip_ = 0.f;
    chockT_ = 0.f;
    chockCd_ = 0.f;
    barT_ = 0.f;
    barOut_ = 0.f;
    barCaught_ = false;
    tele_ = 0.f;
    hit_ = 0.f;
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
    sys_->setLight(120, 72, 28);
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    gate_ = 28;
    fan_ = held ? 0 : -1;
    fanT_ = 0.f;
    tele_ = 0.f;
    hit_ = 0.f;
    barT_ = 0.f;
    bracing_ = false;
    shouldering_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    if (held) {
        slide_ = 0.f;
        shake_ = 0.35f;
        sys_->rumble(0.2f, 0.45f, 220);
        sys_->setLight(40, 140, 60);
        for (int i = 0; i < 8; i++) burst(64.f + float(i) * 26.f, 118.f);
    } else {
        slide_ = 1.f;
        shake_ = 1.15f;
        sys_->apu.noiseBurst(0.7f, 140.f, 0.4f);
        sys_->rumble(0.85f, 0.2f, 280);
        sys_->setLight(160, 20, 14);
    }
}

Game::Intent Game::intent() const {
    Intent in;
    if (bot_) {
        bool threat = tele_ > 0.f || hit_ > 0.f;
        if (barT_ > 0.f && !barCaught_) in.slam = true;
        if (threat && slip_ <= 0.f) in.lean = side_;
        else if (!threat && slide_ > 0.18f && slip_ <= 0.f && arms_ > 0.22f) in.shoulder = true;
        if (!threat && chockT_ <= 0.f && chockCd_ <= 0.f && slide_ > 0.30f) in.chock = true;
        return in;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) in.lean = -1;
    else if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) in.lean = 1;
    in.slam = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z);
    in.chock = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X);
    in.shoulder = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_DOWN) || p.accel > 0.45f;
    return in;
}

void Game::update() {
    while (markNext_ < markCount_ && marks_[markNext_].frame <= age_) {
        const Mark& m = marks_[markNext_++];
        if (m.kind == BAR) {
            barT_ = BAR_T;
            barCaught_ = false;
            blip(520.f);
        } else {
            kind_ = m.kind;
            side_ = m.side;
            tele_ = TELE;
            pendingHit_ = (m.kind == LOG) ? HIT_L : HIT_S;
            blip(m.kind == LOG ? 160.f : 280.f);
        }
    }

    Intent in = intent();
    if (in.slam) {
        if (barT_ > 0.f && !barCaught_) {
            barCaught_ = true;
            barT_ = 0.f;
            slide_ = std::max(0.f, slide_ - 0.02f);
            blip(860.f);
            sys_->apu.noiseBurst(0.16f, 700.f, 0.05f);
            burst(206.f, 128.f);
        } else if (!bot_) {
            blip(96.f);
        }
    }

    bool threat = tele_ > 0.f || hit_ > 0.f;
    if (in.chock) {
        bool can = chockT_ <= 0.f && chockCd_ <= 0.f && slide_ < 0.45f && !threat;
        if (can) {
            chockT_ = CHOCK_T;
            chockCd_ = CHOCK_CD;
            slide_ = std::max(0.f, slide_ - 0.02f);
            blip(210.f);
            sys_->apu.noiseBurst(0.18f, 260.f, 0.06f);
            burst(118.f, 188.f);
        } else if (!bot_ && chockT_ <= 0.f) {
            blip(80.f);
        }
    }

    bool bracing = threat && in.lean == side_ && side_ != 0 && arms_ > 0.02f && slip_ <= 0.f;
    bool shouldering = in.shoulder && !threat && arms_ > 0.02f && slip_ <= 0.f;
    if (slip_ > 0.f) {
        slip_ -= DT;
        if (slip_ < 0.f) slip_ = 0.f;
        arms_ = std::min(1.f, arms_ + 0.12f * DT);
        bracing = false;
        shouldering = false;
    } else {
        float drain = (bracing ? 0.09f : 0.f) + (shouldering ? 0.045f : 0.f);
        if (drain > 0.f) {
            arms_ -= drain * DT;
            if (arms_ <= 0.f) {
                arms_ = 0.f;
                slip_ = 0.5f;
                bracing = false;
                shouldering = false;
                blip(90.f);
            }
        } else {
            arms_ = std::min(1.f, arms_ + 0.16f * DT);
        }
    }
    bracing_ = bracing;
    shouldering_ = shouldering;
    lean_ = in.lean;

    float push = CREEP;
    if (hit_ > 0.f) {
        float base = bracing_ ? ((kind_ == LOG) ? 0.075f : 0.05f) : ((kind_ == LOG) ? 0.72f : 0.55f);
        if (lean_ != 0 && lean_ != side_) base += 0.10f;
        if (chockT_ > 0.f) base *= 0.20f;
        if (barOut_ > 0.f) base *= 1.5f;
        push += base;
    }
    if (shouldering_) push -= SHOULDER;
    slide_ += push * DT;

    if (chockT_ > 0.f) {
        chockT_ -= DT;
        if (chockT_ < 0.f) chockT_ = 0.f;
    }
    if (chockCd_ > 0.f) {
        chockCd_ -= DT;
        if (chockCd_ < 0.f) chockCd_ = 0.f;
    }
    if (barT_ > 0.f) {
        barT_ -= DT;
        if (barT_ <= 0.f) {
            barT_ = 0.f;
            if (!barCaught_) {
                slide_ += BAR_MISS;
                barOut_ = 1.5f;
                shake_ = std::max(shake_, 0.9f);
                blip(110.f);
                sys_->apu.noiseBurst(0.4f, 200.f, 0.14f);
                burst(200.f, 120.f);
            }
        }
    }
    if (barOut_ > 0.f) {
        barOut_ -= DT;
        if (barOut_ < 0.f) barOut_ = 0.f;
    }
    if (tele_ > 0.f) {
        tele_ -= DT;
        if (tele_ <= 0.f) {
            tele_ = 0.f;
            hit_ = pendingHit_;
            recoil_ = 1.f;
            shake_ = std::max(shake_, bracing_ ? 0.28f : 0.8f);
            if (!bracing_) slide_ += (kind_ == LOG) ? 0.07f : 0.04f;
            sys_->apu.noiseBurst(bracing_ ? 0.2f : 0.5f, bracing_ ? 190.f : 120.f, 0.12f);
            sys_->rumble(bracing_ ? 0.22f : 0.7f, 0.15f, 70);
            burst(side_ < 0 ? 100.f : 224.f, 148.f);
        }
    } else if (hit_ > 0.f) {
        hit_ -= DT;
        if (hit_ < 0.f) hit_ = 0.f;
    }

    if (slide_ < 0.f) slide_ = 0.f;
    if (slide_ >= 1.f) {
        finish(false);
        return;
    }

    if ((bracing_ || shouldering_) && (age_ % 12 == 0)) burst(bracing_ && side_ < 0 ? 120.f : 168.f, 186.f);

    float hz = 74.f + slide_ * 210.f + (hit_ > 0.f ? 36.f : 0.f) + (bracing_ ? 22.f : 0.f);
    float vol = 0.018f + slide_ * 0.07f + (bracing_ ? 0.02f : 0.f);
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, vol);

    age_++;
    if (age_ >= HOLD) {
        finish(true);
        return;
    }
    if (age_ % 60 == 0) {
        bool late = age_ >= HOLD - 600;
        sys_->apu.tone(2, late ? 720.f : 330.f, late ? 0.05f : 0.03f);
        tick_ = 0.04f;
    }
}

void Game::fanfare() {
    if (fan_ < 0) return;
    fanT_ += DT;
    if (fan_ < 4 && fanT_ >= 0.16f) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        sys_->apu.tone(1, notes[fan_], 0.07f);
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
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(8, 6, 4));
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    gate_ = 18;
    slide_ = 0.f;
    arms_ = 1.f;
    sys.setLight(120, 72, 28);
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.045f);
    if (recoil_ > 0.f) recoil_ = std::max(0.f, recoil_ - 0.08f);

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
            creakOn();
        }
    } else {
        if (mode_ == Mode::Won) fanfare();
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) toTitle();
    }
    if (mode_ != Mode::Pause) tickSparks();
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
        float gh = float(g.h) * scale;
        spr(g, x + gw * 0.5f, y, gh, pal);
        x += gw;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    stamp(m, cx, cy, h * float(m.w) / float(m.h), h, pal, flip, fog, shadow);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip, int fog, bool shadow) {
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
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sky(float shx) {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = false;
    v.B.enabled = false;
    float danger = (mode_ == Mode::Lost) ? 1.f : std::clamp(slide_, 0.f, 1.f) * 0.45f;
    float warm = (mode_ == Mode::Won) ? 0.6f : 0.f;
    uint16_t top = gs::rgb4(4, 6, 9);
    uint16_t mid = gs::rgb4(8, 7, 8);
    uint16_t low = gs::rgb4(13, 8, 4);
    if (warm > 0.f) {
        top = mixC(top, gs::rgb4(8, 6, 3), warm);
        mid = mixC(mid, gs::rgb4(12, 8, 3), warm);
        low = mixC(low, gs::rgb4(15, 11, 4), warm);
    }
    if (danger > 0.f) {
        top = mixC(top, gs::rgb4(6, 2, 2), danger * 0.45f);
        mid = mixC(mid, gs::rgb4(10, 3, 2), danger * 0.55f);
        low = mixC(low, gs::rgb4(12, 3, 2), danger * 0.7f);
    }
    const float span = float(gs::SCREEN_H) - HORIZON;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            v.road[y].on = false;
            float u = float(y) / HORIZON;
            uint16_t c = u < 0.5f ? mixC(top, mid, u / 0.5f) : mixC(mid, low, (u - 0.5f) / 0.5f);
            v.lineBackdrop[y] = c;
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - HORIZON) / span;
        t = std::max(t, 0.02f);
        float wz = ZNEAR / t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx * t;
        rd.hw = std::max(10.f, 3.05f * 78.f * t);
        rd.v = wz * 14.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 0;
        rd.band = (int(std::floor(wz * 0.35f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((wz - 5.f) / 16.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 7.f);
        v.lineBackdrop[y] = gs::rgb4(6, 4, 2);
    }
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(8, 3, 2) : gs::rgb4(9, 7, 4));
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(40, 140, 60);
    else if (mode_ == Mode::Lost) sys_->setLight(160, 20, 14);
    else if (mode_ == Mode::Title) sys_->setLight(120, 72, 28);
    else if (barT_ > 0.f || slide_ > 0.55f) sys_->setLight(160, 48, 16);
    else if (bracing_ || shouldering_) sys_->setLight(150, 96, 32);
    else sys_->setLight(110, 68, 26);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float shx = 0.f, shy = 0.f;
    if (shake_ > 0.f) {
        shx = std::sin(float(age_ + sys_->frame) * 1.8f) * shake_ * 5.f;
        shy = std::cos(float(age_ + sys_->frame) * 2.2f) * shake_ * 2.4f;
    }
    sky(shx);
    lamp();

    const bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    bool threat = live && (tele_ > 0.f || hit_ > 0.f);
    float pressure = 0.f;
    if (tele_ > 0.f) pressure = 1.f - tele_ / TELE;
    else if (hit_ > 0.f) pressure = 1.f;

    float show = slide_;
    if (mode_ == Mode::Title) show = 0.02f + 0.012f * std::sin(float(sys_->frame) * 0.05f);
    else if (mode_ == Mode::Won) show = 0.f;
    else if (mode_ == Mode::Lost) show = 1.f;
    show = std::clamp(show, 0.f, 1.f);
    float leafW = std::max(48.f, DOOR_W0 * (1.f - 0.62f * show));
    float leafL = DOOR_L0 + 78.f * show + recoil_ * 7.f;
    float leafR = leafL + leafW;
    float leafCx = leafL + leafW * 0.5f;
    float leafCy = DOOR_TOP + DOOR_H * 0.5f + shy;

    float t = float(sys_->frame);
    int wave = (sys_->frame / 10) & 1;
    int step = (sys_->frame / 8) & 1;
    bool flick = ((sys_->frame / 7) & 1) != 0;

    auto X = [&](float x) { return x + shx; };

    if (mode_ == Mode::Title) {
        text("S3 YARD DOOR", 160.f, 16.f, 0.72f, PAL_GOLD);
        text("HOLD THE DOOR", 160.f, 38.f, 0.5f, PAL_TEXT);
        text("THREE MINUTES", 160.f, 54.f, 0.46f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160.f, 18.f, 0.64f, PAL_GOLD);
        text("THREE MINUTES", 160.f, 40.f, 0.5f, PAL_GOOD);
    } else if (mode_ == Mode::Lost) {
        text("THE DOOR OPENED", 160.f, 18.f, 0.52f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 18.f, 0.8f, PAL_GOLD);
    } else {
        int remain = std::max(0, HOLD - age_);
        int sec = (remain + 59) / 60;
        char clock[12];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        text(clock, 160.f, 16.f, 1.15f, remain <= 600 ? PAL_ALERT : PAL_GOLD);
        const char* hint = "HOLD THE DOOR";
        int hintPal = PAL_GOLD;
        if (barT_ > 0.f) {
            hint = "SLAM THE BAR";
            hintPal = PAL_ALERT;
        } else if (threat && kind_ == LOG) {
            hint = side_ < 0 ? "LOG LEFT" : "LOG RIGHT";
            hintPal = PAL_ALERT;
        } else if (threat) {
            hint = side_ < 0 ? "BRACE LEFT" : "BRACE RIGHT";
            hintPal = PAL_ALERT;
        } else if (slide_ > 0.55f) {
            hint = "SHUT THE DOOR";
            hintPal = PAL_ALERT;
        } else if (chockT_ > 0.f) {
            hint = "CHOCK IS IN";
            hintPal = PAL_GOOD;
        } else if (slide_ > 0.18f) {
            hint = "SHOULDER IT SHUT";
            hintPal = PAL_GOLD;
        }
        float hs = hintPal == PAL_ALERT ? 0.64f + std::sin(t * 0.35f) * 0.03f : 0.52f;
        text(hint, 160.f, 40.f, hs, hintPal);
        int n = std::clamp(int(std::lround(arms_ * 8.f)), 0, 8);
        int ap = n <= 2 ? PAL_ALERT : (n <= 5 ? PAL_GOLD : PAL_GOOD);
        for (int i = 0; i < 8; i++) spr(art_.pip, 252.f + float(i) * 8.f, 12.f, i < n ? 7.f : 4.f, i < n ? ap : PAL_WOOD);
    }

    for (const Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        spr(art_.spark, s.x + shx, s.y + shy, 3.f + s.life * 12.f, PAL_DUST);
    }

    float hx = 164.f;
    float hy = 168.f;
    int hf = 0;
    bool hflip = false;
    if (mode_ == Mode::Lost) {
        hx = 214.f;
        hy = 176.f;
    } else if (bracing_) {
        hf = 1;
        hx = side_ < 0 ? 132.f : 196.f;
        hflip = side_ > 0;
        hy = 170.f;
    } else if (shouldering_) {
        hf = 1;
        hx = 146.f;
        hy = 172.f;
    } else if (slip_ > 0.f) {
        hx = 188.f;
        hy = 174.f;
    }
    spr(art_.shadow, X(hx), 206.f + shy, 16.f, PAL_TEXT, false, 0, true);
    if (mode_ != Mode::Title) spr(art_.hand[hf], X(hx), hy + shy, mode_ == Mode::Lost ? 70.f : 80.f, PAL_COAT, hflip);

    if (chockT_ > 0.f) spr(art_.chock, X(leafL + 10.f), DOOR_TOP + DOOR_H + shy, 18.f, PAL_WOOD);

    float barY = DOOR_TOP + 54.f + shy;
    if (barT_ > 0.f) barY -= (1.f - barT_ / BAR_T) * 28.f;
    else if (barOut_ > 0.f) barY -= 22.f;
    stamp(art_.bar, X(leafR - 18.f), barY, 34.f, 8.f, PAL_IRON);

    if (threat) {
        float px = side_ < 0 ? 68.f : 252.f;
        float ph = 64.f + std::sin(t * 0.45f) * 8.f;
        stamp(art_.stripe, X(px), 132.f + shy, 8.f, ph, PAL_ALERT);
    }

    float bob = std::sin(t * 0.08f) * 2.f;
    spr(art_.hoist, X(160.f), 102.f + bob + shy, 40.f, PAL_IRON);
    spr(art_.pennant[wave], X(96.f), 60.f + shy, 16.f, mode_ == Mode::Won ? PAL_GOOD : PAL_ALERT);
    spr(art_.pennant[wave ^ 1], X(224.f), 58.f + shy, 14.f, mode_ == Mode::Won ? PAL_GOOD : PAL_GOLD, true);
    spr(art_.lantern, X(68.f), 98.f + shy, 20.f, PAL_IRON);
    spr(art_.lantern, X(252.f), 98.f + shy, 20.f, PAL_IRON, true);
    spr(art_.flame, X(68.f), 86.f + shy, flick ? 12.f : 8.f, PAL_FIRE);
    spr(art_.flame, X(252.f), 86.f + shy, flick ? 8.f : 11.f, PAL_FIRE);

    spr(art_.post, X(68.f), 138.f + shy, 136.f, PAL_WOOD);
    spr(art_.post, X(252.f), 138.f + shy, 136.f, PAL_WOOD, true);
    stamp(art_.beam, X(160.f), 74.f + shy, 236.f, 18.f, PAL_WOOD);
    stamp(art_.shadow, X(leafCx), DOOR_TOP + DOOR_H + 6.f + shy, leafW * 0.85f, 12.f, PAL_TEXT, false, 0, true);
    stamp(art_.leaf, X(leafCx), leafCy, leafW, DOOR_H, PAL_WOOD);

    if (live && kind_ == LOG && threat) {
        float edge = side_ < 0 ? leafL : leafR;
        float from = side_ < 0 ? -30.f : 350.f;
        float logX = from + (edge - from) * pressure;
        spr(art_.log, X(logX), 154.f + shy, 20.f + pressure * 10.f, PAL_WOOD, side_ > 0);
    }

    auto breakerAt = [&](float x, float foot, float h, int fr, bool flip) {
        spr(art_.breaker[fr], X(x), foot - h * 0.5f + shy, h, PAL_BREAK, flip, 1);
    };
    if (mode_ == Mode::Lost) {
        breakerAt(112.f, 204.f, 68.f, step, false);
        breakerAt(156.f, 206.f, 60.f, step ^ 1, true);
    } else {
        float lPush = (threat && side_ < 0) ? pressure * 16.f : 0.f;
        float rPush = (threat && side_ > 0) ? pressure * 16.f : 0.f;
        breakerAt(44.f + lPush, 206.f, 54.f + lPush * 0.15f, (threat && side_ < 0) ? step : 0, false);
        breakerAt(276.f - rPush, 206.f, 52.f + rPush * 0.15f, (threat && side_ > 0) ? step : 0, true);
    }

    stamp(art_.rail, X(160.f), 198.f + shy, 230.f, 7.f, PAL_IRON);
    spr(art_.horse, X(108.f), 208.f + shy, 26.f, PAL_WOOD);
    spr(art_.barrow, X(230.f), 210.f + shy, 22.f, PAL_WOOD, true);
    spr(art_.coil, X(28.f), 204.f + shy, 18.f, PAL_PROP);
    spr(art_.keg, X(298.f), 202.f + shy, 26.f, PAL_WOOD);

    float cribH = 150.f;
    spr(art_.crib, X(30.f), 214.f - cribH * 0.5f + shy, cribH, PAL_WOOD);
    spr(art_.crib, X(292.f), 214.f - cribH * 0.5f + shy, cribH, PAL_WOOD, true);

    float drift = std::sin(t * 0.02f) * 8.f;
    spr(art_.cloud, X(52.f + drift), 26.f, 16.f, PAL_SKY);
    spr(art_.cloud, X(150.f + drift * 0.6f), 18.f, 12.f, PAL_SKY, true);
    spr(art_.sun, X(236.f), 30.f, 22.f, PAL_SKY);

    if (mode_ == Mode::Title) {
        hudC(20, "ONE YARD", PAL_GOLD);
        hudC(21, "HOLD THE DOOR FOR THREE MINUTES", PAL_TEXT);
        hudC(22, "THEN IT IS DONE", PAL_GOOD);
        hudC(24, "ARROWS BRACE THE STILE", PAL_TEXT);
        hudC(25, "Z BAR   X CHOCK   C SHUT", PAL_GOLD);
        hudC(26, "ENTER", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hud(1, 0, "YARD", PAL_GOLD);
        hudC(25, "ENTER RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Won) {
        hud(1, 0, "YARD", PAL_GOOD);
        hudC(22, "THE DOOR HELD FOR THREE MINUTES", PAL_GOOD);
        hudC(23, "THE YARD IS SHUT", PAL_TEXT);
        hudC(26, "ENTER", PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        hud(1, 0, "YARD", PAL_ALERT);
        hudC(22, "THE DOOR OPENED", PAL_ALERT);
        hudC(23, "THE WATCH IS OVER", PAL_TEXT);
        hudC(26, "ENTER RETRIES", PAL_TEXT);
    } else {
        hud(1, 0, "YARD", PAL_GOLD);
        hud(31, 0, "ARMS", arms_ < 0.28f ? PAL_ALERT : PAL_TEXT);
        hudC(26, "Z BAR   X CHOCK   C SHUT", PAL_TEXT);
    }
}

}  // namespace yarddoor

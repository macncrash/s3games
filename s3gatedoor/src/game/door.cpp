#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace gatedoor {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int HOLD = 180 * 60;
constexpr int HAND = 1;
constexpr int RAM = 2;
constexpr float HAND_T = 1.35f;
constexpr float MISS = 0.20f;
constexpr float LATCH_PUSH = 2.6f;
constexpr float LATCH_CLOSE = 0.42f;
constexpr float WEDGE_T = 2.5f;
constexpr float WEDGE_CD = 8.f;
constexpr float WEDGE_SWING = 0.22f;
constexpr float RAM_TELE = 64.f;

constexpr float HORIZON = 102.f;
constexpr float ZNEAR = 2.2f;
constexpr float PPM = 100.f;
constexpr float ROAD_HALF = 3.2f;
constexpr float PIER_L = 62.f;
constexpr float PIER_R = 258.f;
constexpr float PIER_H = 160.f;
constexpr float PIER_FOOT = 216.f;
constexpr float LEAF_L = 84.f;
constexpr float LEAF_W = 140.f;
constexpr float LEAF_TOP = 72.f;
constexpr float LEAF_H = 124.f;

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float openVis(float s) {
    s = std::clamp(s, 0.f, 1.f);
    if (s <= 0.08f) return s / 0.08f * 0.04f;
    float u = (s - 0.08f) / 0.92f;
    return 0.04f + 0.96f * std::pow(u, 0.75f);
}

gs::FMPatch creakPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.35f;
    p.op[0] = {1.f, 0.7f, 0.05f, 0.4f, 0.8f, 0.45f};
    p.op[1] = {2.01f, 0.3f, 0.08f, 0.5f, 0.5f, 0.4f};
    p.op[2] = {0.5f, 0.4f, 0.1f, 0.45f, 0.7f, 0.35f};
    p.op[3] = {1.5f, 0.2f, 0.06f, 0.3f, 0.4f, 0.3f};
    p.vol = 0.05f;
    p.drive = 0.2f;
    p.tone = 420.f;
    p.glide = 0.01f;
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
    sys_->apu.tone(0, freq, 0.06f);
    blip_ = 0.05f;
}

void Game::burst(float x, float y) {
    for (Spark& s : sparks_) {
        if (s.life > 0.f) continue;
        float ang = float(age_ % 9) * 0.7f;
        s.x = x;
        s.y = y;
        s.vx = std::sin(ang) * 36.f;
        s.vy = -22.f - float(age_ % 4) * 6.f;
        s.life = 0.32f;
        return;
    }
}

int Game::upcoming(int kind) const {
    for (int i = markNext_; i < markCount_; i++)
        if (marks_[i].kind == kind) return marks_[i].frame - age_;
    return -1;
}

void Game::schedule() {
    markCount_ = 0;
    markNext_ = 0;
    auto add = [&](float sec, int kind) {
        if (markCount_ >= int(sizeof marks_ / sizeof marks_[0])) return;
        marks_[markCount_++] = {int(std::lround(sec * 60.f)), kind};
    };
    for (int i = 0; i < 15; i++) {
        add(6.f + float(i) * 11.f, HAND);
        add(12.f + float(i) * 11.f, RAM);
    }
    add(171.f, HAND);
    add(173.5f, RAM);
    add(176.2f, HAND);
    add(178.4f, RAM);
    std::sort(marks_, marks_ + markCount_, [](const Mark& a, const Mark& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.kind < b.kind;
    });
}

void Game::begin() {
    age_ = 0;
    swing_ = 0.f;
    grip_ = 1.f;
    hand_ = 0.f;
    latch_ = 0.f;
    wedge_ = 0.f;
    wedgeCd_ = 0.f;
    slip_ = 0.f;
    shake_ = 0.f;
    recoil_ = 0.f;
    blip_ = 0.f;
    tick_ = 0.f;
    fanT_ = 0.f;
    fan_ = -1;
    hooked_ = false;
    bracing_ = false;
    won_ = false;
    over_ = false;
    gate_ = 12;
    for (Spark& s : sparks_) s.life = 0.f;
    schedule();
    mode_ = Mode::Play;
    sys_->apu.setPatch(0, creakPatch());
    sys_->apu.keyOn(0, 96.f, 0.04f);
    sys_->setLight(110, 64, 28);
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    gate_ = 24;
    fan_ = held ? 0 : -1;
    fanT_ = 0.f;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(2, 0, 0);
    if (held) {
        swing_ = 0.f;
        latch_ = 0.f;
        hand_ = 0.f;
        sys_->rumble(0.2f, 0.5f, 240);
        sys_->setLight(48, 140, 64);
    } else {
        swing_ = 1.f;
        shake_ = 1.1f;
        sys_->apu.noiseBurst(0.7f, 160.f, 0.4f);
        sys_->rumble(0.85f, 0.2f, 280);
        sys_->setLight(160, 18, 14);
    }
}

void Game::toTitle() {
    mode_ = Mode::Title;
    hand_ = 0.f;
    latch_ = 0.f;
    wedge_ = 0.f;
    recoil_ = 0.f;
    shake_ = 0.f;
    swing_ = 0.f;
    gate_ = 14;
    fan_ = -1;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->setLight(36, 28, 84);
}

Game::Intent Game::intent(bool ramHit) {
    Intent in;
    if (bot_) {
        int ramIn = upcoming(RAM);
        bool ramSoon = ramHit || (ramIn >= 0 && ramIn <= 68);
        if ((hand_ > 0.f && hand_ <= 0.88f && !hooked_) || latch_ > 0.f) in.hook = true;
        bool canWedge = wedge_ <= 0.f && wedgeCd_ <= 0.f && swing_ < WEDGE_SWING && slip_ <= 0.f;
        if (canWedge && (grip_ < 0.42f || (ramSoon && grip_ < 0.7f))) in.wedge = true;
        if (slip_ > 0.f) in.hold = false;
        else if (ramHit || latch_ > 0.f || hand_ > 0.f || ramSoon) in.hold = true;
        else if (swing_ > 0.10f) in.hold = true;
        else if (swing_ < 0.045f && grip_ < 0.9f) in.hold = false;
        else if (grip_ > 0.6f && swing_ > 0.055f) in.hold = true;
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.hold = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_DOWN) || p.accel > 0.45f;
    in.hook = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z);
    in.wedge = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X);
    return in;
}

void Game::update() {
    bool ramHit = false;
    bool handStart = false;
    while (markNext_ < markCount_ && marks_[markNext_].frame <= age_) {
        if (marks_[markNext_].kind == RAM) ramHit = true;
        if (marks_[markNext_].kind == HAND) handStart = true;
        markNext_++;
    }
    if (handStart && hand_ <= 0.f && latch_ <= 0.f) {
        hand_ = HAND_T;
        hooked_ = false;
        blip(620.f);
    }

    Intent in = intent(ramHit);

    if (in.hook) {
        if (hand_ > 0.f && !hooked_) {
            hooked_ = true;
            hand_ = 0.f;
            swing_ = std::max(0.f, swing_ - 0.02f);
            blip(880.f);
            sys_->apu.noiseBurst(0.12f, 900.f, 0.04f);
        } else if (latch_ > 0.f) {
            latch_ = 0.f;
            blip(480.f);
            sys_->apu.noiseBurst(0.16f, 420.f, 0.05f);
        }
    }

    if (hand_ > 0.f) {
        hand_ -= DT;
        if (hand_ <= 0.f) {
            hand_ = 0.f;
            if (!hooked_) {
                latch_ = 1.f;
                swing_ += MISS;
                shake_ = std::max(shake_, 0.7f);
                blip(130.f);
                sys_->apu.noiseBurst(0.35f, 240.f, 0.12f);
                burst(LEAF_L + LEAF_W, 140.f);
            }
        }
    }

    if (in.wedge) {
        if (wedge_ <= 0.f && wedgeCd_ <= 0.f && swing_ < WEDGE_SWING && slip_ <= 0.f) {
            wedge_ = WEDGE_T;
            wedgeCd_ = WEDGE_CD;
            swing_ = std::max(0.f, swing_ - 0.03f);
            blip(220.f);
            sys_->apu.noiseBurst(0.2f, 280.f, 0.06f);
            burst(LEAF_L + 120.f, 190.f);
        } else if (wedge_ <= 0.f) {
            blip(80.f);
        }
    }

    if (slip_ > 0.f) {
        slip_ -= DT;
        grip_ = std::min(1.f, grip_ + 0.32f * DT);
        bracing_ = false;
    } else if (in.hold && grip_ > 0.02f) {
        bracing_ = true;
        grip_ -= 0.10f * DT;
        if (grip_ <= 0.f) {
            grip_ = 0.f;
            slip_ = 0.48f;
            bracing_ = false;
        }
    } else {
        bracing_ = false;
        grip_ = std::min(1.f, grip_ + 0.32f * DT);
    }

    float siege = std::clamp(age_ / float(HOLD), 0.f, 1.f);
    float push = (0.125f + 0.055f * siege) * DT;
    float close = (0.36f + 0.14f * siege) * DT;
    if (latch_ > 0.f) {
        push *= LATCH_PUSH;
        close *= LATCH_CLOSE;
    }
    if (wedge_ > 0.f) push *= 0.08f;
    if (bracing_) swing_ -= close;
    if (wedge_ > 0.f) {
        grip_ = std::min(1.f, grip_ + 0.28f * DT);
        swing_ -= 0.04f * DT;
        wedge_ -= DT;
        if (wedge_ < 0.f) wedge_ = 0.f;
    }
    if (wedgeCd_ > 0.f) wedgeCd_ -= DT;
    swing_ += push;

    if (ramHit) {
        float spike = 0.20f;
        if (wedge_ > 0.f) spike = 0.02f;
        else if (bracing_ && grip_ > 0.12f) spike = 0.035f;
        else if (bracing_) spike = 0.08f;
        swing_ += spike;
        shake_ = 1.f;
        recoil_ = 0.30f;
        sys_->apu.noiseBurst(0.55f, 150.f, 0.16f);
        sys_->rumble(0.75f, 0.3f, 90);
        burst(LEAF_L + LEAF_W - 8.f, 150.f);
    }

    if (recoil_ > 0.f) recoil_ -= DT;
    for (Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        s.life -= DT;
        s.x += s.vx * DT;
        s.y += s.vy * DT;
        s.vy += 50.f * DT;
    }
    if (bracing_ && age_ % 14 == 0) burst(158.f, 200.f);

    swing_ = std::max(0.f, swing_);
    float hz = 88.f + swing_ * 240.f + (bracing_ ? 28.f : 0.f);
    float vol = 0.02f + std::min(swing_, 1.f) * 0.08f + (bracing_ ? 0.025f : 0.f);
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, vol);

    if (age_ > 0 && age_ % 60 == 0) {
        bool late = age_ >= HOLD - 600;
        sys_->apu.tone(2, late ? 740.f : 392.f, late ? 0.05f : 0.028f);
        tick_ = 0.04f;
    }

    if (swing_ >= 1.f) {
        swing_ = 1.f;
        finish(false);
        return;
    }
    age_++;
    if (age_ >= HOLD) finish(true);
}

void Game::fanfare() {
    if (fan_ < 0) return;
    fanT_ += DT;
    if (fan_ < 4 && fanT_ >= 0.15f) {
        static const float notes[] = {392.f, 523.f, 659.f, 784.f};
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
    sys.vdp.setFogColor(gs::rgb4(2, 2, 5));
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    sys.setLight(36, 28, 84);
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.03f);
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) ||
                 pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (mode_ == Mode::Title) {
        if (gate_ == 0 && start) begin();
    } else if (mode_ == Mode::Play) {
        if (gate_ == 0 && pad.pressed(gs::BTN_START) && !bot_) {
            mode_ = Mode::Pause;
            sys.apu.keyOff(0);
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            gate_ = 8;
            sys.apu.setPatch(0, creakPatch());
            sys.apu.keyOn(0, 96.f, 0.04f);
        }
    } else {
        if (mode_ == Mode::Won) fanfare();
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) toTitle();
    }
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
        if (c <= 32 || c >= 128) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) {
            x += 10.f * scale;
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
    float warm = mode_ == Mode::Won ? 0.35f : 0.f;
    float hot = mode_ == Mode::Lost ? 0.45f : std::clamp(swing_, 0.f, 1.f) * 0.18f;
    float flick = 0.5f + 0.5f * std::sin(float(sys_->frame) * 0.21f);
    uint16_t top = gs::rgb4(1, 1, 4);
    uint16_t mid = gs::rgb4(3, 2, 6);
    uint16_t low = gs::rgb4(5, 3, 5);
    low = mixC(low, gs::rgb4(8, 4, 2), flick * 0.18f);
    if (warm > 0.f) {
        top = mixC(top, gs::rgb4(6, 4, 3), warm);
        mid = mixC(mid, gs::rgb4(10, 6, 3), warm);
        low = mixC(low, gs::rgb4(12, 7, 3), warm);
    }
    if (hot > 0.f) {
        top = mixC(top, gs::rgb4(5, 1, 2), hot);
        mid = mixC(mid, gs::rgb4(8, 2, 2), hot);
        low = mixC(low, gs::rgb4(6, 1, 1), hot);
    }
    const float span = float(gs::SCREEN_H) - HORIZON;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            v.road[y].on = false;
            float u = y / HORIZON;
            uint16_t c = u < 0.55f ? mixC(top, mid, u / 0.55f) : mixC(mid, low, (u - 0.55f) / 0.45f);
            v.lineBackdrop[y] = c;
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - HORIZON) / span;
        t = std::max(t, 0.018f);
        float wz = ZNEAR / t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx * t;
        rd.hw = std::max(6.f, ROAD_HALF * PPM * t);
        rd.v = wz * 22.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 0;
        rd.band = (int(std::floor(wz * 0.32f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((wz - 7.f) / 16.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11.f);
        v.lineBackdrop[y] = gs::rgb4(1, 2, 1);
    }
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(6, 2, 2) : gs::rgb4(2, 2, 5));
}

void Game::lamp() {
    if (mode_ == Mode::Won) sys_->setLight(48, 140, 64);
    else if (mode_ == Mode::Lost) sys_->setLight(160, 18, 14);
    else if (mode_ == Mode::Title) sys_->setLight(36, 28, 84);
    else if (latch_ > 0.f || swing_ > 0.55f) sys_->setLight(150, 28, 16);
    else if (bracing_) sys_->setLight(130, 78, 28);
    else sys_->setLight(70, 46, 24);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float shx = 0.f, shy = 0.f;
    if (shake_ > 0.f) {
        shx = std::sin(age_ * 1.9f) * shake_ * 6.f;
        shy = std::cos(age_ * 2.3f) * shake_ * 3.f;
    }
    sky(shx);
    lamp();

    const bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    float vis = 0.f;
    if (live) vis = openVis(swing_);
    else if (mode_ == Mode::Lost) vis = 1.f;

    float leafLeft = LEAF_L + shx;
    float leafW = std::max(18.f, LEAF_W * (1.f - 0.78f * vis));
    float leafTop = LEAF_TOP + shy;
    float leafCx = leafLeft + leafW * 0.5f;
    float leafCy = leafTop + LEAF_H * 0.5f;
    float leafRight = leafLeft + leafW;

    int ramIn = live ? upcoming(RAM) : -1;
    bool telegraph = ramIn >= 0 && ramIn <= int(RAM_TELE);
    int torch = int(sys_->frame / 8) & 1;
    bool showHand = live && hand_ > 0.f;
    bool showLatchWord = live && (showHand || latch_ > 0.f);

    if (mode_ == Mode::Title) {
        text("S3 GATE DOOR", 160, 16, 1.0f, PAL_GOLD);
        text("HOLD THE DOOR", 160, 34, 0.62f, PAL_HUD);
        text("THREE MINUTES", 160, 50, 0.48f, PAL_GOLD);
    } else if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160, 18, 0.66f, PAL_GOLD);
        text("THREE MINUTES", 160, 38, 0.5f, PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        text("THE DOOR OPENED", 160, 18, 0.55f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 20, 0.8f, PAL_GOLD);
    } else {
        int remain = std::max(0, HOLD - age_);
        int sec = remain == 0 ? 0 : (remain + 59) / 60;
        char clock[12];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        text(clock, 160, 16, 1.25f, remain <= 600 ? PAL_ALERT : PAL_GOLD);
        if (showLatchWord) text("LATCH", 108, 36, 0.62f, PAL_ALERT);
        if (telegraph) text("RAM", 214, 36, 0.7f, PAL_ALERT);
    }

    for (const Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        spr(art_.spark, s.x + shx, s.y, 3.f + s.life * 10.f, PAL_FIRE);
    }

    if (showHand) spr(art_.hand, leafRight + 10.f, 138.f + shy, 16.f, PAL_RAIDER, true);

    float kx = (bracing_ ? 148.f : 164.f) + shx + (slip_ > 0.f ? 8.f : 0.f);
    float ky = 210.f + shy + (bracing_ ? 2.f : 0.f);
    if (mode_ == Mode::Lost) kx = 188.f + shx;
    const gs::Mipped& body = bracing_ ? art_.keeper[1] : art_.keeper[0];
    if (mode_ != Mode::Title) spr(body, kx, ky - 40.f, bracing_ ? 84.f : 80.f, PAL_COAT);

    if (live && wedge_ > 0.f) spr(art_.wedge, leafRight - 6.f, 192.f + shy, 16.f, PAL_WOOD);

    float barX0 = leafRight - 4.f;
    float barX1 = (latch_ > 0.f || vis > 0.22f) ? barX0 + 14.f : 236.f + shx;
    float barW = std::max(12.f, barX1 - barX0);
    stamp(art_.latch, (barX0 + barX1) * 0.5f, 140.f + shy, barW, 8.f, PAL_IRON);

    int lampPal = mode_ == Mode::Won ? PAL_GOLD : PAL_FIRE;
    spr(art_.lantern[torch], PIER_L + shx, 98.f, 22.f, lampPal);
    spr(art_.lantern[torch], PIER_R + shx, 98.f, 22.f, lampPal, true);

    float bellX = 160.f + shx;
    if (mode_ == Mode::Won) bellX += std::sin(float(sys_->frame) * 0.45f) * 7.f;
    spr(art_.bell, bellX, 82.f + shy, 16.f, PAL_IRON);

    spr(art_.hinge, 80.f + shx, 100.f + shy, 16.f, PAL_IRON);
    spr(art_.hinge, 80.f + shx, 142.f + shy, 16.f, PAL_IRON);
    spr(art_.hinge, 80.f + shx, 178.f + shy, 14.f, PAL_IRON);
    spr(art_.catchPlate, 236.f + shx, 140.f + shy, 20.f, PAL_IRON);

    float pierCy = PIER_FOOT - PIER_H * 0.5f + shy;
    spr(art_.pier, PIER_L + shx, pierCy, PIER_H, PAL_STONE);
    spr(art_.pier, PIER_R + shx, pierCy, PIER_H, PAL_STONE, true);
    stamp(art_.beam, 160.f + shx, 64.f + shy, 214.f, 18.f, PAL_STONE);

    stamp(art_.leaf, leafCx, leafCy, leafW, LEAF_H, PAL_WOOD);

    auto raiderAt = [&](float x, float foot, float h, int frame, bool front) {
        (void)front;
        spr(art_.raider[frame], x, foot - h * 0.5f, h, PAL_RAIDER, false, 2);
    };
    int step = int(sys_->frame / 10) & 1;
    if (mode_ == Mode::Lost) {
        raiderAt(132.f + shx, 206.f, 64.f, step, true);
        raiderAt(176.f + shx, 208.f, 58.f, step ^ 1, true);
    } else {
        raiderAt(leafRight + 8.f, 194.f, 46.f, step, false);
        raiderAt(std::min(leafRight + 28.f, 248.f), 196.f, 38.f, step ^ 1, false);
    }

    float approach = 0.f;
    if (telegraph) approach = 1.f - float(ramIn) / RAM_TELE;
    if (recoil_ > 0.f) approach = std::clamp(recoil_ / 0.30f, 0.f, 1.f);
    if (live && approach > 0.02f) {
        float ramX = 310.f - approach * (310.f - (leafRight + 6.f));
        spr(art_.ram, ramX, 156.f + shy, 22.f + approach * 8.f, PAL_WOOD, false, 1);
    }

    spr(art_.tree, 18.f, 150.f, 40.f, PAL_GRASS, false, 6);
    spr(art_.tree, 304.f, 154.f, 34.f, PAL_GRASS, true, 6);
    spr(art_.tuft, 24.f + shx, 206.f, 10.f, PAL_GRASS);
    spr(art_.tuft, 300.f + shx, 208.f, 9.f, PAL_GRASS);
    spr(art_.tuft, 46.f + shx, 200.f, 8.f, PAL_GRASS);

    spr(art_.moon, 28.f, 22.f, 18.f, PAL_MOON);
    static const int stars[][2] = {{70, 12}, {96, 22}, {130, 8}, {188, 14}, {210, 6}, {236, 18}, {280, 10}, {300, 24}};
    for (int i = 0; i < 8; i++) {
        if (mode_ == Mode::Won && (i % 2) == 0) continue;
        float tw = ((sys_->frame / 12 + i) % 6 == 0) ? 2.5f : 4.f;
        spr(art_.star, float(stars[i][0]), float(stars[i][1]), tw, PAL_MOON);
    }

    if (mode_ != Mode::Title) {
        spr(art_.chip, kx, 214.f, 8.f, PAL_COAT, false, 0, true);
    }

    if (live) {
        float left = 8.f;
        float tw = 70.f;
        float gw = tw * std::clamp(grip_, 0.f, 1.f);
        int barPal = grip_ < 0.28f ? PAL_ALERT : (bracing_ ? PAL_GOLD : PAL_GOOD);
        stamp(art_.chip, left + tw * 0.5f, 12.f, tw, 5.f, PAL_STONE);
        if (gw > 1.f) stamp(art_.chip, left + gw * 0.5f, 12.f, gw, 3.f, barPal);
        hud(1, 0, "GRIP", PAL_HUD);
        const char* state = "REST";
        int spal = PAL_HUD;
        if (latch_ > 0.f) {
            state = "LATCH UP";
            spal = PAL_ALERT;
        } else if (slip_ > 0.f) {
            state = "SLIP";
            spal = PAL_ALERT;
        } else if (wedge_ > 0.f) {
            state = "WEDGE";
            spal = PAL_GOOD;
        } else if (bracing_) {
            state = "HOLD";
            spal = PAL_GOLD;
        }
        hud(1, 26, state, spal);
        hud(12, 27, "Z LATCH  X WEDGE  C HOLD", PAL_HUD);
    } else if (mode_ == Mode::Title) {
        hudC(23, "Z LATCHES THE BAR", PAL_GOLD);
        hudC(24, "X WEDGES THE LEAF", PAL_HUD);
        hudC(25, "C OR SPACE HOLDS IT", PAL_HUD);
        if ((sys_->frame / 30) & 1) hudC(27, "ENTER", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(27, "ENTER", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(26, "THE GATE IS OPEN", PAL_ALERT);
    } else if (mode_ == Mode::Won) {
        hudC(26, "THE WATCH IS DONE", PAL_GOLD);
    }
}

}  // namespace gatedoor

#include "game/keep.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace keep {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float OPEN_L = 86.f;
constexpr float OPEN_R = 236.f;
constexpr float OPEN_T = 42.f;
constexpr float OPEN_B = 186.f;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch creakPatch() {
    gs::FMPatch p;
    p.alg = 2;
    p.fb = 0.45f;
    p.op[0] = {1.0f, 0.8f, 0.04f, 0.35f, 0.85f, 0.4f};
    p.op[1] = {2.0f, 0.35f, 0.08f, 0.4f, 0.6f, 0.35f};
    p.op[2] = {0.5f, 0.45f, 0.1f, 0.5f, 0.7f, 0.4f};
    p.op[3] = {1.0f, 0.25f, 0.06f, 0.3f, 0.5f, 0.3f};
    p.vol = 0.07f;
    p.drive = 0.25f;
    p.tone = 480;
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
    beep_ = 0.045f;
}

void Game::burst(float x, float y) {
    for (Spark& s : sparks_) {
        if (s.life > 0) continue;
        float ang = float(age_ % 7) * 0.9f;
        s.x = x;
        s.y = y;
        s.vx = std::sin(ang) * 28.f;
        s.vy = -18.f - float(age_ % 5) * 4.f;
        s.life = 0.35f;
        return;
    }
}

void Game::schedule() {
    markCount_ = 0;
    markNext_ = 0;
    auto add = [&](float sec, int kind) {
        if (markCount_ >= int(sizeof marks_ / sizeof marks_[0])) return;
        marks_[markCount_++] = {int(std::lround(sec * 60.f)), kind};
    };
    for (int i = 0; i < 12; i++) add(12.f + i * 14.f, CROW);
    for (int i = 0; i < 10; i++) add(20.f + i * 16.f, RAM);
    add(170.f, CROW);
    add(171.4f, RAM);
    std::sort(marks_, marks_ + markCount_, [](const Mark& a, const Mark& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.kind < b.kind;
    });
}

void Game::begin() {
    age_ = 0;
    score_ = 0;
    slams_ = 0;
    braced_ = 0;
    gap_ = 0;
    bar_ = 1;
    stam_ = 1;
    lean_ = 0;
    crow_ = 0;
    ram_ = 0;
    braceCd_ = 0;
    braceArm_ = 0;
    shake_ = 0;
    beep_ = 0;
    recoil_ = 0;
    pushing_ = false;
    crowAnswered_ = false;
    botBraced_ = false;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    gate_ = 12;
    for (Spark& s : sparks_) s.life = 0;
    schedule();
    mode_ = Mode::Play;
    sys_->apu.setPatch(0, creakPatch());
    sys_->apu.keyOn(0, 78.f, 0.06f);
    sys_->setLight(90, 50, 20);
}

void Game::hold(bool kept) {
    mode_ = kept ? Mode::Won : Mode::Lost;
    won_ = kept;
    over_ = true;
    gate_ = 20;
    fanStep_ = kept ? 0 : -1;
    fanT_ = 0;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    if (kept) {
        sys_->rumble(0.15f, 0.45f, 220);
        sys_->setLight(40, 90, 40);
    } else {
        gap_ = 1;
        sys_->apu.noiseBurst(0.65f, 280.f, 0.45f);
        sys_->rumble(0.8f, 0.2f, 260);
        sys_->setLight(90, 12, 10);
    }
}

Game::Intent Game::intent() {
    Intent in;
    if (bot_) {
        bool ramSoon = ram_ > 0 && ram_ < 0.65f;
        bool ramHit = ram_ > 0 && ram_ < 0.36f;
        in.slam = (crow_ > 0.f && !crowAnswered_) || bar_ < 0.92f;
        if (in.slam && crow_ > 0.f) crowAnswered_ = true;
        if (crow_ <= 0.f) crowAnswered_ = false;
        bool danger = gap_ > 0.03f || ramSoon || bar_ < 0.75f || crow_ > 0.2f;
        bool gasp = stam_ < 0.1f && gap_ < 0.08f && !ramSoon && bar_ > 0.85f;
        in.shoulder = danger && !gasp;
        in.brace = ramHit && !botBraced_ && braceCd_ <= 0.f && stam_ > 0.22f;
        if (in.brace) botBraced_ = true;
        if (ram_ <= 0.f) botBraced_ = false;
        in.lean = in.shoulder ? 1.f : 0.f;
        return in;
    }
    const gs::Pad& p = sys_->pad;
    in.shoulder = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.accel > 0.35f;
    in.slam = p.pressed(gs::BTN_B);
    in.brace = p.pressed(gs::BTN_A);
    float lean = p.axisX;
    if (p.down(gs::BTN_LEFT)) lean -= 1.f;
    if (p.down(gs::BTN_RIGHT)) lean += 1.f;
    in.lean = std::clamp(lean, -1.f, 1.f);
    return in;
}

void Game::update() {
    while (markNext_ < markCount_ && age_ >= marks_[markNext_].frame) {
        int kind = marks_[markNext_].kind;
        if (kind == CROW && crow_ <= 0.f) {
            crow_ = 2.4f;
            blip(196.f);
        } else if (kind == RAM && ram_ <= 0.f) {
            ram_ = 1.2f;
            blip(110.f);
        }
        markNext_++;
    }

    Intent in = intent();
    lean_ += (in.lean - lean_) * 0.18f;

    if (in.slam && (crow_ > 0.f || bar_ < 0.995f)) {
        if (crow_ > 0.f) slams_++;
        crow_ = 0;
        bar_ = 1.f;
        blip(320.f);
        sys_->apu.noiseBurst(0.22f, 500.f, 0.06f);
    }
    if (in.brace && braceCd_ <= 0.f && stam_ > 0.2f) {
        braceCd_ = 1.55f;
        braceArm_ = 0.5f;
        stam_ = std::max(0.f, stam_ - 0.18f);
        gap_ = std::max(0.f, gap_ - 0.045f);
        blip(520.f);
        shake_ = std::max(shake_, 0.45f);
    }

    if (crow_ > 0.f) {
        crow_ -= DT;
        if (crow_ < 0.f) crow_ = 0;
        bar_ = std::max(0.f, bar_ - DT * 0.48f);
    }
    if (braceCd_ > 0.f) braceCd_ -= DT;
    if (braceArm_ > 0.f) {
        gap_ -= 0.5f * DT;
        braceArm_ -= DT;
    }

    pushing_ = false;
    if (in.shoulder && stam_ > 0.004f) {
        float mul = 1.f + 0.12f * in.lean;
        gap_ -= 0.38f * mul * DT;
        stam_ = std::max(0.f, stam_ - 0.25f * DT);
        pushing_ = true;
    } else {
        stam_ = std::min(1.f, stam_ + 0.17f * DT);
    }

    float siege = std::clamp(age_ / float(HOLD), 0.f, 1.f);
    float press = 0.055f + 0.125f * siege;
    if (age_ > 165 * 60) press += 0.04f;
    press *= 1.f + 0.08f * std::sin(age_ * 0.07f);
    float cover = 1.f;
    if (bar_ > 0.82f) cover = 0.15f;
    else if (bar_ > 0.4f) cover = 0.42f;
    gap_ += press * cover * DT;

    if (age_ > 90 && age_ % 240 == 0) {
        float bump = (0.012f + 0.02f * siege) * (bar_ > 0.8f ? 0.35f : 1.f);
        gap_ += bump;
        shake_ = std::max(shake_, 0.28f);
        sys_->apu.noiseBurst(0.12f, 700.f, 0.04f);
    }

    if (ram_ > 0.f) {
        ram_ -= DT;
        shake_ = std::max(shake_, (1.2f - std::max(ram_, 0.f)) * 0.25f);
        if (ram_ <= 0.f) {
            ram_ = 0;
            float spike = bar_ > 0.8f ? 0.09f : 0.22f;
            if (pushing_) spike *= 0.55f;
            if (braceArm_ > 0.f) {
                spike *= 0.28f;
                braced_++;
            }
            gap_ += spike;
            shake_ = 1.f;
            recoil_ = 0.32f;
            sys_->apu.noiseBurst(0.55f, 180.f, 0.18f);
            sys_->rumble(0.7f, 0.35f, 90);
            burst(OPEN_R - 20.f, (OPEN_T + OPEN_B) * 0.5f);
        }
    }
    if (recoil_ > 0.f) recoil_ -= DT;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.035f);

    for (Spark& s : sparks_) {
        if (s.life <= 0) continue;
        s.life -= DT;
        s.x += s.vx * DT;
        s.y += s.vy * DT;
        s.vy += 40.f * DT;
    }

    gap_ = std::clamp(gap_, 0.f, 1.f);
    float hz = 72.f + gap_ * 160.f + (pushing_ ? 24.f : 0.f);
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, 0.045f + gap_ * 0.09f);

    age_++;
    score_ = age_ / 6 + slams_ * 25 + braced_ * 40;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0, 0);
    } else if (age_ % 60 == 0 && age_ < HOLD) {
        blip(age_ > HOLD - 600 ? 680.f : 392.f);
    }

    if (gap_ >= 1.f) hold(false);
    else if (age_ >= HOLD) hold(true);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 1, 2));
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        bool go = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) ||
                  pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
        if (go) begin();
    } else if (mode_ == Mode::Play) {
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            gate_ = 8;
        }
    } else if (fanStep_ >= 0) {
        fanT_ += DT;
        if (fanT_ >= 0.16f && fanStep_ < 4) {
            fanT_ = 0;
            static const float notes[] = {262.f, 330.f, 392.f, 523.f};
            sys.apu.tone(1, notes[fanStep_], 0.07f);
            fanStep_++;
            beep_ = 0.12f;
        } else if (fanStep_ >= 4 && beep_ > 0.f) {
            beep_ -= DT;
            if (beep_ <= 0.f) sys.apu.tone(1, 0, 0);
        }
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) {
            sys.apu.tone(1, 0, 0);
            mode_ = Mode::Title;
            gate_ = 10;
        }
    } else if (gate_ == 0 && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Title;
        gate_ = 10;
    }
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    const float adv = 18.0f * scale;
    x -= n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    stamp(m, cx, cy, h * float(m.w) / float(m.h), h, pal, flip);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    float flick = 0.5f + 0.5f * std::sin(sys_->frame * 0.17f + age_ * 0.05f);
    float cold = (mode_ == Mode::Lost) ? 0.85f : gap_ * 0.65f;
    uint16_t top = lerpC(gs::rgb4(2, 2, 4), gs::rgb4(1, 1, 3), cold);
    uint16_t mid = lerpC(gs::rgb4(6, 5, 5), gs::rgb4(2, 2, 5), cold);
    mid = lerpC(mid, gs::rgb4(8, 5, 3), flick * 0.35f * (1.f - cold));
    uint16_t bot = lerpC(gs::rgb4(3, 2, 2), gs::rgb4(1, 1, 3), cold);
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        uint16_t c = t < 0.55f ? lerpC(top, mid, t / 0.55f) : lerpC(mid, bot, (t - 0.55f) / 0.45f);
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    float shx = 0, shy = 0;
    if (shake_ > 0.f) {
        shx = std::sin(age_ * 1.7f) * shake_ * 7.f;
        shy = std::cos(age_ * 2.1f) * shake_ * 3.f;
    }

    bool showCrow = crow_ > 0.f;
    bool crowSoon = false;
    if (!showCrow && markNext_ < markCount_ && marks_[markNext_].kind == CROW) {
        int left = marks_[markNext_].frame - age_;
        crowSoon = left >= 0 && left < 40;
    }

    // Sprites: the first one written stays on top, so the clock and calls go first.
    if (mode_ == Mode::Title) {
        text("S3 KEEP", 160, 16, 1.05f, PAL_RED);
        text("THE DOOR", 160, 38, 0.72f, PAL_GOLD);
        text("FOR THREE MINUTES", 160, 196, 0.48f, PAL_WARN);
    } else if (mode_ != Mode::Pause) {
        int remain = std::max(0, HOLD - age_);
        int sec = remain == 0 ? 0 : (remain + 59) / 60;
        char clock[16];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        text(clock, 160, 16, 1.15f, (remain <= 600 && mode_ == Mode::Play) ? PAL_RED : PAL_GOLD);
    }
    if (mode_ == Mode::Play && (showCrow || crowSoon)) text("CROW", 160, 52, 0.7f, PAL_WARN);
    if (mode_ == Mode::Play && ram_ > 0.f) text("RAM", 160, showCrow || crowSoon ? 72.f : 52.f, 0.85f, PAL_RED);
    if (mode_ == Mode::Pause) text("PAUSED", 160, 100, 1.0f, PAL_HUD);
    if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160, 78, 0.62f, PAL_GOLD);
        text("THREE MINUTES", 160, 100, 0.55f, PAL_HUD);
    }
    if (mode_ == Mode::Lost) text("THE DOOR OPENS", 160, 88, 0.62f, PAL_RED);
    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        float fw = 70.f * stam_;
        stamp(art_.chip, 118, 214, 74, 6, PAL_TRACK, false);
        if (fw > 1.f) stamp(art_.chip, 83 + fw * 0.5f, 214, fw, 4, stam_ < 0.28f ? PAL_RED : PAL_GOLD, false);
    }

    float vis = 0.02f;
    if (mode_ != Mode::Title) vis = std::clamp(0.04f + gap_ * 0.96f, 0.f, 1.f);
    if (mode_ == Mode::Lost) vis = 1.f;
    float openW = OPEN_R - OPEN_L;
    float openH = OPEN_B - OPEN_T;
    float leafW = (openW - 10.f) * (1.f - 0.62f * vis);
    float leafH = openH - 6.f;
    float leafCx = OPEN_L + leafW * 0.5f + 4.f + shx;
    float leafCy = (OPEN_T + OPEN_B) * 0.5f + shy;
    int torch = int(sys_->frame / 8) & 1;

    for (const Spark& s : sparks_) {
        if (s.life <= 0) continue;
        spr(art_.mote, s.x + shx, s.y, 4.f + s.life * 8.f, PAL_FIRE, false);
    }

    spr(art_.torch[torch], OPEN_L - 8 + shx, OPEN_T + 36, 36, PAL_FIRE, false);
    spr(art_.torch[torch], OPEN_R + 8 + shx, OPEN_T + 48, 32, PAL_FIRE, true);

    const gs::Mipped& body = pushing_ || braceArm_ > 0.f ? art_.keeperShove : art_.keeper;
    float kx = 160.f + lean_ * 16.f + shx;
    spr(body, kx, 162 + shy, pushing_ ? 86.f : 80.f, PAL_CLOAK, lean_ < -0.15f);

    float barY = leafCy - 6.f - (1.f - bar_) * 46.f;
    float barX = leafCx + (1.f - bar_) * 28.f;
    if (showCrow || crowSoon) spr(art_.crow, barX + 36.f, barY - 8.f, 16.f, PAL_IRON, false);
    if (bar_ > 0.72f) spr(art_.bar, barX, barY, 16.f, PAL_IRON, false);
    else spr(art_.barTilt, barX + 8.f, barY - 10.f, 28.f + (1.f - bar_) * 10.f, PAL_IRON, false);

    spr(art_.floor, 160 + shx, 206, 34, PAL_STONE, false);

    if (ram_ > 0.f || recoil_ > 0.f) {
        float u = ram_ > 0.f ? 1.f - ram_ / 1.2f : 1.f;
        float rx = OPEN_R + 30.f - u * (36.f + vis * 40.f) + (recoil_ > 0 ? recoil_ * 40.f : 0);
        spr(art_.ram, rx + shx, leafCy + 6, 26.f + u * 10.f, PAL_OAK, false);
    }

    spr(art_.jamb, OPEN_L - 6 + shx, leafCy, openH + 8, PAL_STONE, false);
    spr(art_.jamb, OPEN_R + 6 + shx, leafCy, openH + 8, PAL_STONE, true);
    spr(art_.lintel, 160 + shx, OPEN_T - 8, 20, PAL_STONE, false);
    spr(art_.bracket, OPEN_L + 18 + shx, leafCy - 6, 20, PAL_IRON, false);
    spr(art_.bracket, OPEN_R - 18 + shx, leafCy - 6, 20, PAL_IRON, true);

    float crackW = 3.f + vis * 16.f;
    stamp(art_.crack, leafCx + leafW * 0.22f, leafCy, crackW, leafH * 0.78f, PAL_FIRE, false);
    stamp(art_.door, leafCx, leafCy, leafW, leafH, PAL_OAK, false);

    int hands = 1 + int(std::clamp(age_ / float(HOLD), 0.f, 1.f) * 3.f);
    if (mode_ == Mode::Title) hands = 1;
    float gapX = OPEN_L + leafW + 8.f;
    for (int i = 0; i < hands; i++) {
        float hy = OPEN_T + 36.f + i * 28.f;
        float hx = gapX + (i & 1) * 8.f + std::sin(age_ * 0.2f + i) * 3.f;
        spr(art_.hand, hx + shx, hy, 16.f + (i == 0 ? 4.f : 0.f), PAL_CLOAK, i & 1);
    }
    stamp(art_.night, (OPEN_L + OPEN_R) * 0.5f + shx, leafCy, openW - 8.f, openH - 4.f, PAL_NIGHT, false);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        const char* bar = bar_ > 0.82f ? "BAR SET" : bar_ > 0.35f ? "BAR RISING" : "BAR OFF";
        hud(1, 25, bar, bar_ > 0.82f ? PAL_GOLD : PAL_RED);
        hud(1, 26, "SHOULDER", stam_ < 0.28f ? PAL_RED : PAL_HUD);
        char sc[16];
        std::snprintf(sc, sizeof sc, "%d", score_);
        hud(40 - int(std::strlen(sc)), 26, sc, PAL_HUD);
        if (age_ < 180) hud(6, 27, "Z BRACE   X BAR   C SHOULDER", PAL_HUD);
    }

    if (mode_ == Mode::Title) {
        hud(8, 22, "Z BRACE THE RAM", PAL_GOLD);
        hud(8, 23, "X SLAM THE BAR", PAL_HUD);
        hud(8, 24, "C OR SPACE  SHOULDER", PAL_HUD);
        hud(7, 25, "ARROWS LEAN INTO IT", PAL_HUD);
        if ((sys_->frame / 30) & 1) hud(14, 27, "ENTER START", PAL_GOLD);
    }
}

}  // namespace keep

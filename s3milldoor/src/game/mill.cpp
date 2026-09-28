#include "game/mill.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace mill {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int HOLD = 180 * 60;
constexpr int SURGE = 1;
constexpr int CHOCK = 2;
constexpr int TELE_F = 32;
constexpr int STRIKE_F = 40;

constexpr float DOOR_L = 124.f;
constexpr float DOOR_R = 214.f;
constexpr float DOOR_T = 50.f;
constexpr float DOOR_B = 186.f;

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

struct Wish {
    int lean = 0;
    bool bar = false;
    bool chock = false;
};

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::note(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    noteT_ = 0.06f;
}

void Game::fanfare() {
    if (fan_ < 0) return;
    static const float notes[] = {196.f, 247.f, 294.f, 392.f, 494.f};
    fanT_ -= DT;
    if (fanT_ > 0.f) return;
    if (fan_ >= 5) {
        fan_ = -1;
        sys_->apu.tone(1, 0, 0);
        return;
    }
    sys_->apu.tone(1, notes[fan_], 0.08f);
    fanT_ = 0.16f;
    fan_++;
}

void Game::layCues() {
    cueCount_ = 0;
    cueNext_ = 0;
    auto add = [&](double sec, int kind, int side) {
        if (cueCount_ >= int(sizeof cues_ / sizeof cues_[0])) return;
        cues_[cueCount_++] = Cue{int(std::lround(sec * 60.0)), kind, side};
    };
    double t = 6.0;
    int i = 0;
    while (t < 168.0) {
        if (i % 3 == 2) add(t, CHOCK, 0);
        else add(t, SURGE, (i & 1) ? 1 : -1);
        t += 7.2 - 2.4 * (t / 180.0);
        i++;
    }
    add(170.5, SURGE, -1);
    add(173.2, CHOCK, 0);
    add(175.6, SURGE, 1);
    add(177.4, SURGE, -1);
    std::sort(cues_, cues_ + cueCount_, [](const Cue& a, const Cue& b) { return a.frame < b.frame; });
}

void Game::begin() {
    age_ = 0;
    gap_ = 0.f;
    shake_ = 0.f;
    wheel_ = 0.f;
    lean_ = 0;
    kind_ = 0;
    side_ = 0;
    barred_ = false;
    chocked_ = false;
    phase_ = Phase::Quiet;
    cueNext_ = 0;
    fan_ = -1;
    fanT_ = 0.f;
    noteT_ = 0.f;
    won_ = false;
    over_ = false;
    gate_ = 8;
    for (int i = 0; i < 20; i++) {
        motes_[i].x = float(12 + (i * 37) % 290);
        motes_[i].y = float((i * 19) % 200);
        motes_[i].vy = 12.f + float(i % 5) * 4.f;
        motes_[i].life = 0.f;
    }
    layCues();
    mode_ = Mode::Play;
    sys_->setLight(120, 90, 30);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    gap_ = 0.f;
    phase_ = Phase::Quiet;
    lean_ = 0;
    kind_ = 0;
    barred_ = false;
    fan_ = -1;
    gate_ = 12;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.noise(0, 0, false);
    sys_->setLight(40, 50, 70);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.HUD.enabled = true;
    toTitle();
}

void Game::update() {
    wheel_ += DT * (1.1f + gap_ * 1.4f);
    if (noteT_ > 0.f) {
        noteT_ -= DT;
        if (noteT_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT);

    if (phase_ == Phase::Quiet && cueNext_ < cueCount_ && age_ >= cues_[cueNext_].frame) {
        kind_ = cues_[cueNext_].kind;
        side_ = cues_[cueNext_].side;
        cueNext_++;
        phase_ = Phase::Tele;
        chocked_ = false;
        note(kind_ == CHOCK ? 180.f : 140.f);
    }

    Wish w;
    if (bot_) {
        w.bar = true;
        w.lean = (kind_ == SURGE && phase_ != Phase::Quiet) ? side_ : 0;
        w.chock = kind_ == CHOCK && phase_ == Phase::Strike && !chocked_;
    } else {
        const gs::Pad& p = sys_->pad;
        w.bar = p.down(gs::BTN_A) || p.down(gs::BTN_C) || p.down(gs::BTN_Z);
        if (p.down(gs::BTN_LEFT) && !p.down(gs::BTN_RIGHT)) w.lean = -1;
        else if (p.down(gs::BTN_RIGHT) && !p.down(gs::BTN_LEFT)) w.lean = 1;
        w.chock = p.pressed(gs::BTN_B);
    }
    barred_ = w.bar;
    lean_ = w.lean;

    if (phase_ == Phase::Tele) {
        age_++;
        if (age_ - cues_[cueNext_ - 1].frame >= TELE_F) phase_ = Phase::Strike;
    } else if (phase_ == Phase::Strike) {
        int into = age_ - cues_[cueNext_ - 1].frame - TELE_F;
        if (kind_ == SURGE) {
            if (w.bar && w.lean == side_) gap_ = std::max(0.f, gap_ - 0.0035f);
            else {
                gap_ += 0.020f;
                shake_ = std::max(shake_, 0.35f);
            }
        } else {
            if (w.chock && !chocked_) {
                chocked_ = true;
                gap_ = std::max(0.f, gap_ - 0.04f);
                note(320.f);
            }
            if (into == STRIKE_F - 1 && !chocked_) {
                gap_ += 0.32f;
                shake_ = 0.8f;
                sys_->apu.noiseBurst(0.2f, 1800.f, 0.2f);
            }
        }
        age_++;
        if (into + 1 >= STRIKE_F) {
            phase_ = Phase::Quiet;
            kind_ = 0;
            side_ = 0;
        }
    } else {
        if (w.bar) gap_ = std::max(0.f, gap_ - 0.0014f);
        else gap_ += 0.00045f;
        age_++;
    }

    if (gap_ >= 1.f) {
        gap_ = 1.f;
        mode_ = Mode::Lost;
        over_ = true;
        won_ = false;
        gate_ = 20;
        sys_->apu.noiseBurst(0.35f, 400.f, 0.4f);
        sys_->setLight(160, 30, 16);
        return;
    }
    if (age_ >= HOLD) {
        gap_ = 0.f;
        mode_ = Mode::Won;
        over_ = true;
        won_ = true;
        gate_ = 30;
        fan_ = 0;
        fanT_ = 0.f;
        sys_->setLight(40, 140, 70);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    if (mode_ == Mode::Title) {
        wheel_ += DT * 0.6f;
        if ((bot_ && gate_ == 0) || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            sys.apu.tone(0, 0, 0);
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else {
        if (mode_ == Mode::Won) fanfare();
        if (gate_ == 0 && sys.pad.pressed(gs::BTN_START)) toTitle();
    }
    for (Mote& m : motes_) {
        m.y -= m.vy * DT;
        m.x += std::sin(m.y * 0.08f + wheel_) * 8.f * DT;
        if (m.y < -4.f) {
            m.y = float(gs::SCREEN_H + (int(m.x) % 20));
            m.x = std::fmod(m.x + 47.f, 300.f) + 8.f;
        }
    }
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
    if (mode_ == Mode::Won) sys_->setLight(30, 140, 70);
    else if (mode_ == Mode::Lost) sys_->setLight(160, 24, 16);
    else if (mode_ == Mode::Title) sys_->setLight(50, 60, 80);
    else if (phase_ == Phase::Strike) sys_->setLight(150, 70, 20);
    else if (barred_) sys_->setLight(140, 100, 30);
    else sys_->setLight(80, 70, 40);
}

void Game::backdrop(float shx) {
    gs::VDP& v = sys_->vdp;
    float danger = (mode_ == Mode::Lost) ? 1.f : std::clamp(gap_, 0.f, 1.f);
    uint16_t sky = mixC(gs::rgb4(3, 5, 8), gs::rgb4(8, 3, 2), danger * 0.7f);
    uint16_t mid = mixC(gs::rgb4(5, 6, 4), gs::rgb4(7, 4, 2), danger * 0.5f);
    uint16_t low = mixC(gs::rgb4(3, 4, 3), gs::rgb4(5, 3, 2), danger * 0.4f);
    if (mode_ == Mode::Won) {
        sky = mixC(sky, gs::rgb4(4, 7, 6), 0.45f);
        mid = mixC(mid, gs::rgb4(5, 8, 4), 0.35f);
    }
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 48) v.lineBackdrop[y] = sky;
        else if (y < 156) v.lineBackdrop[y] = mid;
        else v.lineBackdrop[y] = low;
    }
    v.setFogColor(gs::rgb4(4, 5, 4));
    for (int y = 176; y < gs::SCREEN_H; y++) {
        float t = (float(y) - 176.f) / 48.f;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 70.f + shx * 0.2f;
        rd.hw = 36.f + t * 28.f;
        rd.v = wheel_ * 4.f + t * 18.f;
        rd.pal = PAL_RACE;
        rd.style = 2;
        rd.band = (y / 4) & 1;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        v.lineFog[y] = 1;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(age_) * 1.9f) * shake_ * 5.f;
    backdrop(shx);
    lamp();

    auto X = [&](float x) { return x + shx; };
    float show = gap_;
    if (mode_ == Mode::Title) show = 0.03f;
    else if (mode_ == Mode::Won) show = 0.f;
    else if (mode_ == Mode::Lost) show = 1.f;
    show = std::clamp(show, 0.f, 1.f);

    int wtick = int(wheel_ * 8.f) & 1;
    spr(art_.wheel[wtick], X(52.f), 118.f, 78.f, PAL_WHEEL);
    spr(art_.vane, X(52.f), 70.f, 28.f, PAL_IRON);
    spr(art_.splash, X(36.f), 168.f, 14.f, PAL_WATER);
    spr(art_.splash, X(78.f), 174.f, 12.f, PAL_WATER, true);
    spr(art_.sack, X(250.f), 168.f, 28.f, PAL_FLOUR);
    spr(art_.sack, X(272.f), 174.f, 24.f, PAL_FLOUR, true);
    spr(art_.sack, X(292.f), 162.f, 22.f, PAL_OAK);

    for (const Mote& m : motes_) spr(art_.dust, X(m.x), m.y, 5.f, PAL_FLOUR, false, 8);

    float open = (DOOR_R - DOOR_L) * show;
    float leafL = DOOR_L + open;
    float leafR = DOOR_R;
    float leafCx = (leafL + leafR) * 0.5f;
    float leafW = std::max(8.f, leafR - leafL);
    float leafCy = (DOOR_T + DOOR_B) * 0.5f;
    float leafH = DOOR_B - DOOR_T;

    float mx = lean_ < 0 ? 146.f : (lean_ > 0 ? 196.f : 168.f);
    spr(art_.miller[barred_ ? 1 : 0], X(mx), 158.f, barred_ ? 58.f : 52.f, PAL_APRON, lean_ > 0);
    if (kind_ == CHOCK && phase_ != Phase::Quiet) spr(art_.chock, X(168.f), 188.f, chocked_ ? 12.f : 8.f, PAL_OAK);

    stamp(art_.leaf, X(leafCx), leafCy, leafW, leafH, PAL_OAK);
    spr(art_.post, X(DOOR_L - 10.f), leafCy, leafH + 10.f, PAL_STONE);
    spr(art_.post, X(DOOR_R + 10.f), leafCy, leafH + 10.f, PAL_STONE);
    stamp(art_.beam, X(168.f), DOOR_T - 8.f, 120.f, 16.f, PAL_IRON);

    if (mode_ == Mode::Title) {
        text("S3 MILL DOOR", 160.f, 18.f, 0.58f, PAL_LAMP);
        text("HOLD THE DOOR", 160.f, 40.f, 0.4f, PAL_TEXT);
        text("THREE MINUTES", 160.f, 56.f, 0.38f, PAL_GOOD);
        hudC(24, "BAR A   MEET THE SURGE   CHOCK B", PAL_TEXT);
        hudC(26, "START", PAL_LAMP);
    } else if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160.f, 18.f, 0.52f, PAL_LAMP);
        text("THREE MINUTES", 160.f, 40.f, 0.44f, PAL_GOOD);
        text("THE WATCH IS DONE", 160.f, 58.f, 0.32f, PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160.f, 18.f, 0.46f, PAL_ALERT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 18.f, 0.7f, PAL_LAMP);
    } else {
        int remain = std::max(0, HOLD - age_);
        int sec = (remain + 59) / 60;
        char clock[16];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        text(clock, 200.f, 16.f, 0.5f, remain < 600 ? PAL_ALERT : PAL_LAMP);
        int bars = int(std::lround((1.f - gap_) * 16.f));
        char seam[20];
        for (int i = 0; i < 16; i++) seam[i] = (i < bars) ? '#' : '.';
        seam[16] = 0;
        hud(12, 26, seam, gap_ > 0.55f ? PAL_ALERT : PAL_GOOD);
        if (phase_ != Phase::Quiet && kind_ == CHOCK && !chocked_) hudC(3, "CHOCK", PAL_ALERT);
        else if (phase_ != Phase::Quiet && kind_ == SURGE) hudC(3, side_ < 0 ? "LEFT" : "RIGHT", PAL_LAMP);
        else if (!barred_) hudC(3, "HOLD", PAL_ALERT);
    }
}

}  // namespace mill

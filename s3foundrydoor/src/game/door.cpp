#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace foundrydoor {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int HOLD = 180 * 60;
constexpr float WARN_S = 0.48f;
constexpr float SHOVE_S = 0.62f;
constexpr float HOOK_S = 0.58f;
constexpr float QUENCH_CD = 2.15f;
constexpr float NOTCH_LO = 0.40f;
constexpr float NOTCH_HI = 0.58f;

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

bool Game::notchOpen() const {
    float phase = std::fmod(float(age_) * DT * 0.82f, 1.f);
    return phase >= NOTCH_LO && phase <= NOTCH_HI;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    blip_ = 0.05f;
}

void Game::sparkAt(float x, float y) {
    for (Spark& s : sparks_) {
        if (s.life > 0.f) continue;
        s.x = x;
        s.y = y;
        s.vy = -28.f;
        s.life = 0.35f;
        return;
    }
}

void Game::tickSparks() {
    for (Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        s.life -= DT;
        s.y += s.vy * DT;
        s.vy += 50.f * DT;
    }
}

void Game::layBeats() {
    beatCount_ = 0;
    beatNext_ = 0;
    auto add = [&](double sec, int side) {
        if (beatCount_ >= int(sizeof beats_ / sizeof beats_[0])) return;
        beats_[beatCount_++] = Beat{int(std::lround(sec * 60.0)), side};
    };
    double t = 6.0;
    int i = 0;
    while (t < 168.0) {
        if (i % 4 == 3) add(t, 0);
        else add(t, (i & 1) ? 1 : -1);
        t += 9.2 - 3.1 * (t / 180.0);
        i++;
    }
    add(170.5, -1);
    add(173.2, 0);
    add(175.8, 1);
    add(178.0, -1);
    std::sort(beats_, beats_ + beatCount_, [](const Beat& a, const Beat& b) { return a.frame < b.frame; });
}

void Game::droneOn() {
    sys_->apu.tone(1, 55.f, 0.03f);
}

void Game::begin() {
    age_ = 0;
    seam_ = 0.10f;
    grip_ = 1.f;
    slip_ = 0.f;
    warn_ = 0.f;
    shove_ = 0.f;
    hook_ = 0.f;
    quenchCd_ = 0.4f;
    dogged_ = false;
    bracing_ = false;
    planting_ = false;
    side_ = 0;
    lean_ = 0;
    kind_ = Kind::None;
    shake_ = 0.f;
    blip_ = 0.f;
    tick_ = 0.f;
    steam_ = 0.f;
    fan_ = -1;
    fanT_ = 0.f;
    won_ = false;
    over_ = false;
    gate_ = 8;
    for (Spark& s : sparks_) s.life = 0.f;
    layBeats();
    mode_ = Mode::Play;
    droneOn();
    sys_->setLight(120, 50, 12);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    seam_ = 0.f;
    grip_ = 1.f;
    warn_ = 0.f;
    shove_ = 0.f;
    hook_ = 0.f;
    bracing_ = false;
    planting_ = false;
    shake_ = 0.f;
    fan_ = -1;
    gate_ = 16;
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->setLight(40, 24, 16);
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    gate_ = 24;
    fan_ = held ? 0 : -1;
    fanT_ = 0.f;
    warn_ = 0.f;
    shove_ = 0.f;
    hook_ = 0.f;
    bracing_ = false;
    planting_ = false;
    sys_->apu.tone(1, 0, 0);
    if (held) {
        seam_ = 0.f;
        shake_ = 0.25f;
        sys_->rumble(0.12f, 0.35f, 180);
        sys_->setLight(30, 120, 50);
    } else {
        seam_ = 1.f;
        shake_ = 1.f;
        sys_->apu.noiseBurst(0.7f, 90.f, 0.4f);
        sys_->rumble(0.8f, 0.2f, 240);
        sys_->setLight(160, 30, 8);
    }
}

Game::Wish Game::wish() const {
    Wish w;
    if (bot_) {
        bool threat = warn_ > 0.f || shove_ > 0.f;
        if (hook_ > 0.f && !dogged_) w.dog = true;
        if (threat && slip_ <= 0.f) w.lean = side_;
        else if (!threat && hook_ <= 0.f && quenchCd_ <= 0.f && notchOpen() && seam_ > 0.18f)
            w.quench = true;
        else if (!threat && seam_ > 0.42f && grip_ > 0.55f) w.plant = true;
        return w;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT) || p.axisX < -0.35f) w.lean = -1;
    else if (p.down(gs::BTN_RIGHT) || p.axisX > 0.35f) w.lean = 1;
    w.dog = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Z);
    w.quench = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X);
    w.plant = p.down(gs::BTN_DOWN) || p.down(gs::BTN_C) || p.accel > 0.5f;
    return w;
}

void Game::update() {
    while (beatNext_ < beatCount_ && beats_[beatNext_].frame <= age_) {
        const Beat& b = beats_[beatNext_++];
        if (b.side == 0) {
            hook_ = HOOK_S;
            dogged_ = false;
            kind_ = Kind::Hook;
            blip(520.f);
        } else {
            side_ = b.side;
            kind_ = Kind::Push;
            warn_ = WARN_S;
            blip(b.side < 0 ? 160.f : 210.f);
        }
    }

    Wish w = wish();
    if (w.dog) {
        if (hook_ > 0.f && !dogged_) {
            dogged_ = true;
            hook_ = 0.f;
            seam_ = std::max(0.f, seam_ - 0.012f);
            blip(740.f);
            sparkAt(160.f, 42.f);
        } else if (!bot_) {
            blip(80.f);
        }
    }

    bool threat = warn_ > 0.f || shove_ > 0.f;
    if (w.quench && quenchCd_ <= 0.f) {
        quenchCd_ = QUENCH_CD;
        if (!threat && hook_ <= 0.f && notchOpen()) {
            seam_ = std::max(0.f, seam_ - 0.09f);
            steam_ = 0.35f;
            blip(280.f);
            sys_->apu.noiseBurst(0.12f, 400.f, 0.05f);
            sparkAt(168.f, 150.f);
        } else {
            seam_ += 0.05f;
            steam_ = 0.55f;
            shake_ = std::max(shake_, 0.4f);
            blip(110.f);
            sys_->apu.noiseBurst(0.28f, 160.f, 0.08f);
        }
    }

    bool bracing = threat && w.lean == side_ && side_ != 0 && grip_ > 0.02f && slip_ <= 0.f;
    bool planting = w.plant && !threat && hook_ <= 0.f && grip_ > 0.02f && slip_ <= 0.f;
    if (slip_ > 0.f) {
        slip_ -= DT;
        if (slip_ < 0.f) slip_ = 0.f;
        grip_ = std::min(1.f, grip_ + 0.2f * DT);
        bracing = false;
        planting = false;
    } else {
        float drain = (bracing ? 0.16f : 0.f) + (planting ? 0.12f : 0.f);
        if (drain > 0.f) {
            grip_ -= drain * DT;
            if (grip_ <= 0.f) {
                grip_ = 0.f;
                slip_ = 0.55f;
                bracing = false;
                planting = false;
                blip(70.f);
            }
        } else {
            grip_ = std::min(1.f, grip_ + 0.28f * DT);
        }
    }
    bracing_ = bracing;
    planting_ = planting;
    lean_ = w.lean;

    float late = std::clamp((float(age_) - 140.f * 60.f) / (40.f * 60.f), 0.f, 1.f);
    float push = 0.0034f * (1.f + 2.4f * late);
    if (shove_ > 0.f) push += bracing_ ? 0.02f : 0.62f;
    if (planting_) push -= 0.045f;
    if (steam_ > 0.2f && !notchOpen()) push += 0.01f;
    seam_ += push * DT;

    if (quenchCd_ > 0.f) quenchCd_ = std::max(0.f, quenchCd_ - DT);
    if (steam_ > 0.f) steam_ = std::max(0.f, steam_ - DT);
    if (hook_ > 0.f) {
        hook_ -= DT;
        if (hook_ <= 0.f) {
            hook_ = 0.f;
            if (!dogged_) {
                seam_ += 0.16f;
                shake_ = std::max(shake_, 0.7f);
                blip(96.f);
                sys_->apu.noiseBurst(0.4f, 140.f, 0.12f);
            }
        }
    }
    if (warn_ > 0.f) {
        warn_ -= DT;
        if (warn_ <= 0.f) {
            warn_ = 0.f;
            shove_ = SHOVE_S;
            shake_ = std::max(shake_, bracing_ ? 0.2f : 0.7f);
            if (!bracing_) seam_ += 0.055f;
            sys_->apu.noiseBurst(bracing_ ? 0.16f : 0.46f, bracing_ ? 220.f : 100.f, 0.09f);
            sys_->rumble(bracing_ ? 0.18f : 0.6f, 0.1f, 50);
            sparkAt(side_ < 0 ? 108.f : 212.f, 130.f);
        }
    } else if (shove_ > 0.f) {
        shove_ = std::max(0.f, shove_ - DT);
    }

    if (seam_ < 0.f) seam_ = 0.f;
    if (seam_ >= 1.f) {
        finish(false);
        return;
    }
    sys_->apu.tone(1, 48.f + seam_ * 90.f + (shove_ > 0.f ? 20.f : 0.f), 0.02f + seam_ * 0.04f);

    age_++;
    if (age_ >= HOLD) {
        finish(true);
        return;
    }
    if (age_ % 60 == 0) {
        bool ending = age_ >= HOLD - 600;
        sys_->apu.tone(2, ending ? 660.f : 240.f, ending ? 0.04f : 0.022f);
        tick_ = 0.04f;
    }
}

void Game::fanfare() {
    if (fan_ < 0) return;
    fanT_ += DT;
    if (fan_ < 4 && fanT_ >= 0.16f) {
        static const float notes[] = {330.f, 392.f, 494.f, 659.f};
        sys_->apu.tone(2, notes[fan_], 0.06f);
        fan_++;
        fanT_ = 0.f;
    } else if (fan_ >= 4 && fanT_ > 0.45f) {
        sys_->apu.tone(2, 0, 0);
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
        if (tick_ <= 0.f && fan_ < 0) sys_->apu.tone(2, 0, 0);
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
        for (int x = 0; x < 40; x++) sys.vdp.B.set(x, y, gs::entry(1, PAL_BRICK));
    sys.vdp.hudEnabled = true;
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    gate_ = 18;
    seam_ = 0.f;
    grip_ = 1.f;
    sys.setLight(40, 24, 16);
    backdrop();
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.035f);

    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B);
    if (mode_ == Mode::Title) {
        if (gate_ == 0 && start) begin();
    } else if (mode_ == Mode::Play) {
        if (gate_ == 0 && pad.pressed(gs::BTN_START) && !bot_) {
            mode_ = Mode::Pause;
            sys.apu.tone(1, 0, 0);
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            gate_ = 8;
            droneOn();
        }
    } else {
        if (mode_ == Mode::Won) fanfare();
        if (gate_ == 0 && pad.pressed(gs::BTN_START)) toTitle();
    }
    tickSparks();
    serviceAudio();
    backdrop();
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
    int n = 0;
    while (s[n]) n++;
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -400, 400));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -400, 400));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::backdrop() {
    float hot = std::clamp(seam_, 0.f, 1.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = float(y) / float(gs::SCREEN_H - 1);
        int r = int(1 + t * (4 + hot * 8));
        int g = int(1 + t * (1 + hot * 2));
        int b = 1;
        sys_->vdp.lineBackdrop[y] = gs::rgb4(std::min(r, 15), std::min(g, 15), b);
        sys_->vdp.lineFog[y] = uint8_t(t > 0.72f ? int((t - 0.72f) * 18.f) : 0);
        sys_->vdp.road[y].on = false;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(float(age_) * 0.9f) * 4.f * std::min(shake_, 1.f);

    float gap = (mode_ == Mode::Lost) ? 28.f : seam_ * 22.f;
    if (mode_ == Mode::Won) gap = 0.f;
    float doorX = 132.f - gap * 0.35f + shx;
    spr(art_.door, doorX, 118.f, 150.f, PAL_IRON, false, int(seam_ * 6.f));
    int fr = (age_ / 8) & 1;
    float flameH = 28.f + seam_ * 36.f + (shove_ > 0.f ? 8.f : 0.f);
    spr(art_.flame[fr], doorX + 34.f + gap, 128.f, flameH, PAL_HEAT, false, 0);

    float hookY = 28.f;
    if (hook_ > 0.f) hookY = 28.f + (1.f - hook_ / HOOK_S) * 36.f;
    spr(art_.hook, 168.f + shx, hookY, hook_ > 0.f ? 36.f : 22.f, dogged_ ? PAL_GOOD : PAL_IRON, false, 1);

    float phase = std::fmod(float(age_) * DT * 0.82f, 1.f);
    float ladX = 46.f + phase * 70.f;
    spr(art_.ladle, ladX + shx, 196.f, 16.f, notchOpen() ? PAL_GOOD : PAL_SLAG, false, 0);
    spr(art_.notch, 46.f + (NOTCH_LO + NOTCH_HI) * 0.5f * 70.f + shx, 186.f, 12.f, PAL_EMBER, false, 0);

    float leanX = 196.f + float(lean_) * 8.f;
    if (mode_ == Mode::Lost) {
        spr(art_.founder[0], leanX + 10.f + shx, 150.f, 48.f, PAL_FIGURE, true, 2);
    } else {
        spr(art_.founder[(age_ / 10) & 1], leanX + shx, 142.f, planting_ || bracing_ ? 78.f : 72.f, PAL_FIGURE, false,
            0);
    }
    if (steam_ > 0.f) spr(art_.spark, 150.f + shx, 160.f, 10.f + steam_ * 20.f, PAL_FX, false, 0);
    for (const Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        spr(art_.spark, s.x + shx, s.y, 7.f, PAL_HEAT, false, 0);
    }

    int left = std::max(0, HOLD - age_);
    if (mode_ != Mode::Play && mode_ != Mode::Pause) left = HOLD;
    int sec = left / 60;
    char clock[16];
    std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
    hud(1, 1, "HOLD", PAL_EMBER);
    hud(7, 1, clock, mode_ == Mode::Play && sec < 10 ? PAL_ALERT : PAL_TEXT);

    int bars = int(std::lround(std::clamp(seam_, 0.f, 1.f) * 10.f));
    char seam[16] = "SEAM ";
    for (int i = 0; i < 10; i++) seam[5 + i] = i < bars ? '#' : '.';
    seam[15] = 0;
    hud(22, 1, seam, bars >= 7 ? PAL_ALERT : PAL_HEAT);

    if (warn_ > 0.f) hudC(4, side_ < 0 ? "SLAG  LEFT" : "LATCH  RIGHT", PAL_ALERT);
    else if (hook_ > 0.f && !dogged_) hudC(4, "DOG THE HOOK", PAL_EMBER);
    else if (notchOpen() && mode_ == Mode::Play) hudC(4, "QUENCH", PAL_GOOD);

    if (mode_ == Mode::Title) {
        hudC(18, "YOU HAVE THE FOUNDRY", PAL_EMBER);
        hudC(20, "HOLD THE DOOR THREE MINUTES", PAL_TEXT);
        hudC(22, "ARROWS BRACE THE PUSH", PAL_TEXT);
        hudC(23, "A DOGS THE HOOK", PAL_TEXT);
        hudC(24, "B QUENCHES IN THE NOTCH", PAL_TEXT);
        hudC(26, "START", PAL_GOOD);
    } else if (mode_ == Mode::Pause) {
        hudC(22, "PAUSED", PAL_EMBER);
        hudC(24, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Won) {
        hudC(20, "THE DOOR HELD", PAL_GOOD);
        hudC(22, "THREE MINUTES", PAL_TEXT);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        hudC(20, "THE DOOR OPENED", PAL_ALERT);
        hudC(22, "ANYTHING ELSE IS A LOSS", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else {
        hud(1, 26, bracing_ ? "BRACE" : (planting_ ? "PLANT" : "GRIP"), bracing_ ? PAL_GOOD : PAL_TEXT);
        int g = int(std::lround(grip_ * 8.f));
        char gbar[12] = "        ";
        for (int i = 0; i < 8; i++) gbar[i] = i < g ? '|' : '.';
        hud(7, 26, gbar, grip_ < 0.25f ? PAL_ALERT : PAL_IRON);
    }
}

}  // namespace foundrydoor

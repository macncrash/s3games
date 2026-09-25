#include "game/oven.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace oven {
namespace {

struct Spec {
    float delay;
    float rate;
};

constexpr Spec kSpec[6] = {
    {0.5f, 6.4f},   // ROUND
    {4.2f, 7.6f},   // TIN
    {6.8f, 9.2f},   // SPLIT
    {8.4f, 8.0f},   // PLAIT
    {11.0f, 10.2f}, // BLOOM
    {12.6f, 7.4f},  // COB
};

constexpr float kTurnLo = 50.f;
constexpr float kTurnGold = 72.f;
constexpr float kBurnNear = 78.f;
constexpr float kPullLo = 58.f;
constexpr float kPullGold = 80.f;
constexpr float kBurnFar = 92.f;
constexpr float kFarMul = 0.85f;
constexpr int kLock = 18;

constexpr float kArchY = 82.f;
constexpr float kArchH = 56.f;
constexpr float kLoafY = 86.f;
constexpr float kLoafH = 34.f;
constexpr float kFlameY = 104.f;
constexpr float kFlameH = 22.f;
constexpr float kMeterY = 112.f;
constexpr float kPeelY = 168.f;
constexpr float kBoardY = 190.f;
constexpr float kCrumbY = 182.f;

constexpr const char* kName[6] = {"ROUND", "TIN", "SPLIT", "PLAIT", "BLOOM", "COB"};

struct Demo {
    int look;
    const char* name;
    float near;
    float far;
};

constexpr Demo kDemo[6] = {
    {LOOK_RAW, "DOUGH", 6.f, 0.f},
    {LOOK_PALE, "PALE", 30.f, 0.f},
    {LOOK_TURN, "TURN", 60.f, 0.f},
    {LOOK_BAKE, "BAKE", 64.f, 28.f},
    {LOOK_DRAW, "GOLD", 64.f, 68.f},
    {LOOK_CHAR, "CHAR", 78.f, 92.f},
};

gs::FMPatch clickPatch(float bright, float vol) {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.18f;
    p.op[0] = {1.f, 1.f, 0.006f, 0.16f, 0.12f, 0.08f};
    p.op[1] = {2.f, 0.32f, 0.005f, 0.12f, 0.04f, 0.07f};
    p.op[2] = {3.f, bright, 0.004f, 0.1f, 0.f, 0.06f};
    p.op[3] = {1.f, 0.22f, 0.01f, 0.2f, 0.16f, 0.1f};
    p.vol = vol;
    p.tone = 2200.f;
    p.echo = 0.12f;
    return p;
}

gs::FMPatch dronePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.fb = 0.28f;
    p.op[0] = {1.f, 1.f, 0.06f, 0.4f, 0.8f, 0.4f};
    p.op[1] = {2.f, 0.18f, 0.08f, 0.5f, 0.55f, 0.4f};
    p.op[2] = {0.5f, 0.12f, 0.1f, 0.45f, 0.4f, 0.35f};
    p.op[3] = {1.f, 0.08f, 0.1f, 0.4f, 0.35f, 0.3f};
    p.vol = 0.04f;
    p.tone = 480.f;
    p.echo = 0.22f;
    return p;
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setPatch(0, clickPatch(0.12f, 0.2f));
    sys.apu.setPatch(1, clickPatch(0.45f, 0.18f));
    sys.apu.setPatch(2, dronePatch());
    sys.apu.setPatch(3, clickPatch(0.35f, 0.2f));
    sys.apu.setEcho(0.16f, 0.28f, 0.16f);
    sys.apu.setMaster(0.72f);
    if (bot_) begin();
    else toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    fan_ = -1;
    msgT_ = 0;
    shake_ = 0;
    if (sys_) sys_->apu.silence();
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    drawn_ = 0;
    sel_ = 0;
    lock_ = 0;
    burned_ = -1;
    t_ = 0;
    msgT_ = 0;
    hurryT_ = 0.2f;
    shake_ = 0;
    fan_ = -1;
    hold_ = 0;
    flash_ = 0;
    why_[0] = 0;
    msg_[0] = 0;
    for (auto& L : loaf_) L = {};
    sys_->apu.silence();
    sys_->apu.keyOn(2, 73.4f, 0.035f);
}

float Game::heat(int i) const {
    return kSpec[i].rate * (1.f + 0.05f * std::sin(t_ * 1.6f + float(i) * 0.9f));
}

void Game::say(const char* s) {
    if (bot_) return;
    std::snprintf(msg_, sizeof msg_, "%s", s);
    msgT_ = 0.9f;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.05f);
    blipT_ = 0.05f;
}

bool Game::tryTurn(int i) {
    Loaf& L = loaf_[i];
    if (L.pulled) {
        say("DRAWN");
        return false;
    }
    if (L.turned) {
        say("ALREADY");
        return false;
    }
    if (t_ < kSpec[i].delay || L.near < kTurnLo) {
        say("TOO SOON");
        return false;
    }
    if (L.near >= kBurnNear) return false;
    L.turned = true;
    lock_ = kLock;
    say("TURNED");
    sys_->apu.keyOn(0, 174.f, 0.2f);
    sys_->rumble(0.12f, 0.22f, 40);
    return true;
}

bool Game::tryPull(int i) {
    Loaf& L = loaf_[i];
    if (L.pulled) {
        say("DRAWN");
        return false;
    }
    if (!L.turned) {
        say("TURN FIRST");
        return false;
    }
    if (L.far < kPullLo) {
        say("NOT YET");
        return false;
    }
    if (L.far >= kBurnFar) return false;
    L.pulled = true;
    L.fly = 0;
    drawn_++;
    lock_ = kLock;
    flash_ = 0.12f;
    say("DRAWN");
    sys_->apu.keyOn(1, 523.f, 0.18f);
    sys_->rumble(0.22f, 0.4f, 70);
    return true;
}

void Game::win() {
    mode_ = Mode::Clear;
    over_ = true;
    won_ = true;
    for (auto& L : loaf_) L.fly = 1.f;
    fan_ = 0;
    fanT_ = 0;
    sys_->apu.keyOff(2);
    sys_->rumble(0.28f, 0.5f, 200);
}

void Game::die(int i) {
    mode_ = Mode::Dead;
    over_ = true;
    won_ = false;
    burned_ = i;
    shake_ = 1.f;
    std::snprintf(why_, sizeof why_, "%s burned", kName[i]);
    sys_->apu.keyOff(2);
    sys_->apu.noiseBurst(0.55f, 90.f, 0.45f);
    sys_->apu.keyOn(0, 55.f, 0.28f);
    sys_->rumble(0.85f, 1.f, 280);
}

void Game::bot() {
    int best = -1;
    int act = 0;
    float slack = 1.0e9f;
    for (int i = 0; i < 6; i++) {
        const Loaf& L = loaf_[i];
        if (L.pulled) continue;
        if (!L.turned && L.near >= kTurnLo && L.near < kBurnNear) {
            float s = kBurnNear - L.near;
            if (s < slack) {
                slack = s;
                best = i;
                act = 1;
            }
        } else if (L.turned && L.far >= kPullLo && L.far < kBurnFar) {
            float s = kBurnFar - L.far;
            if (s < slack) {
                slack = s;
                best = i;
                act = 2;
            }
        }
    }
    if (best < 0) return;
    sel_ = best;
    if (act == 1) tryTurn(best);
    else tryPull(best);
}

void Game::human() {
    gs::Pad& pad = sys_->pad;
    bool left = pad.pressed(gs::BTN_LEFT);
    bool right = pad.pressed(gs::BTN_RIGHT);
    if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_RIGHT)) hold_++;
    else hold_ = 0;
    if (!left && !right && hold_ > 14 && (hold_ % 6) == 0) {
        left = pad.down(gs::BTN_LEFT);
        right = pad.down(gs::BTN_RIGHT);
    }
    if (left) sel_ = (sel_ + 5) % 6;
    if (right) sel_ = (sel_ + 1) % 6;
    if (lock_ > 0) return;
    bool turn = pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_Y);
    bool draw = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    if (turn) tryTurn(sel_);
    else if (draw) tryPull(sel_);
}

void Game::step(float dt) {
    for (int i = 0; i < 6; i++) {
        Loaf& L = loaf_[i];
        if (L.pulled || t_ < kSpec[i].delay) continue;
        float h = heat(i) * dt;
        if (!L.turned) {
            L.near += h;
            if (!L.toldTurn && L.near >= kTurnLo) {
                L.toldTurn = true;
                L.spark = 0.5f;
                blip(440.f + float(i) * 28.f);
            }
        } else {
            L.far += h * kFarMul;
            if (!L.toldPull && L.far >= kPullLo) {
                L.toldPull = true;
                L.spark = 0.5f;
                blip(660.f + float(i) * 22.f);
            }
        }
    }
    for (int i = 0; i < 6; i++) {
        const Loaf& L = loaf_[i];
        if (L.pulled) continue;
        if (!L.turned && L.near >= kBurnNear) {
            die(i);
            return;
        }
        if (L.turned && L.far >= kBurnFar) {
            die(i);
            return;
        }
    }
    bool hurry = false;
    for (int i = 0; i < 6; i++) {
        const Loaf& L = loaf_[i];
        if (L.pulled) continue;
        if (!L.turned && L.near >= kTurnGold && L.near < kBurnNear) hurry = true;
        if (L.turned && L.far >= kPullGold && L.far < kBurnFar) hurry = true;
    }
    if (hurry) {
        hurryT_ -= dt;
        if (hurryT_ <= 0.f) {
            blip(920.f);
            hurryT_ = 0.26f;
        }
    } else {
        hurryT_ = 0.2f;
    }
    if (lock_ > 0) lock_--;
    else if (bot_) bot();
    else human();
    if (drawn_ >= 6) {
        win();
        return;
    }
    t_ += dt;
}

int Game::look(int i) const {
    if (i == burned_) return LOOK_CHAR;
    const Loaf& L = loaf_[i];
    if (L.pulled) return LOOK_DRAW;
    if (!L.turned) {
        if (L.near < 16.f) return LOOK_RAW;
        if (L.near < kTurnLo) return LOOK_PALE;
        if (L.near < kTurnGold) return LOOK_TURN;
        if (L.near < kBurnNear) return LOOK_DANGER;
        return LOOK_CHAR;
    }
    if (L.far < 18.f) return LOOK_FLIP;
    if (L.far < kPullLo) return LOOK_BAKE;
    if (L.far < kPullGold) return LOOK_DRAW;
    if (L.far < kBurnFar) return LOOK_DARK;
    return LOOK_CHAR;
}

void Game::hint(char* out, int n, int& pal) const {
    pal = PAL_INK;
    if (msgT_ > 0.f) {
        std::snprintf(out, size_t(n), "%s", msg_);
        return;
    }
    int best = -1;
    float slack = 1.0e9f;
    const char* verb = "WATCH";
    for (int i = 0; i < 6; i++) {
        const Loaf& L = loaf_[i];
        if (L.pulled) continue;
        if (!L.turned && L.near >= kTurnLo && L.near < kBurnNear) {
            float s = kBurnNear - L.near;
            if (s < slack) {
                slack = s;
                best = i;
                verb = L.near < kTurnGold ? "TURN" : "HURRY";
            }
        } else if (L.turned && L.far >= kPullLo && L.far < kBurnFar) {
            float s = kBurnFar - L.far;
            if (s < slack) {
                slack = s;
                best = i;
                verb = L.far < kPullGold ? "DRAW" : "HURRY";
            }
        }
    }
    if (best < 0) {
        std::snprintf(out, size_t(n), "WATCH THE CRUST");
        return;
    }
    if (verb[0] == 'H') pal = PAL_ALERT;
    else pal = PAL_GOLD;
    std::snprintf(out, size_t(n), "%s  %s", verb, kName[best]);
}

void Game::bed(float dt) {
    float fire = 0.018f;
    float rate = 860.f;
    if (mode_ == Mode::Play) fire = 0.028f;
    else if (mode_ == Mode::Dead) {
        fire = 0.04f;
        rate = 240.f;
    } else if (mode_ == Mode::Clear) fire = 0.014f;
    sys_->apu.noise(fire, rate, false);
    if (mode_ == Mode::Play) sys_->apu.setVol(2, 0.03f + 0.01f * std::sin(anim_ * 2.4f));
    if (blipT_ > 0.f) {
        blipT_ -= dt;
        if (blipT_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (fan_ >= 0 && fan_ < 4) {
        fanT_ += dt;
        const float notes[4] = {392.f, 494.f, 587.f, 784.f};
        if (fanT_ >= float(fan_) * 0.13f) {
            sys_->apu.keyOn(3, notes[fan_], 0.2f);
            fan_++;
        }
    }
    if (flash_ > 0.f) flash_ -= dt;
    if (flash_ > 0.f) sys_->setLight(90, 200, 80);
    else if (mode_ == Mode::Dead) sys_->setLight(210, 28, 12);
    else if (mode_ == Mode::Clear) sys_->setLight(48, 170, 64);
    else sys_->setLight(190, 84, 28);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) mode_ = Mode::Pause;
        else step(dt);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) toTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) {
        begin();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        toTitle();
    }
    anim_ += dt;
    if (msgT_ > 0.f) msgT_ -= dt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt);
    if (mode_ != Mode::Title && mode_ != Mode::Pause) {
        for (auto& L : loaf_) {
            if (L.pulled && L.fly < 1.f) L.fly = std::min(1.f, L.fly + dt * 2.4f);
            if (L.spark > 0.f) L.spark = std::max(0.f, L.spark - dt);
        }
    }
    bed(dt);
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 220));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 400));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::solid(float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || art_.solid.h < 1) return;
    gs::Sprite s;
    s.img = art_.solid.lv[0];
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
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

void Game::hudC(int row, const char* s, int pal) {
    if (!s || !s[0]) return;
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::meter(float x, float y, float value, float burn, float z0, float z1) {
    const float w = 40.f;
    float fw = w * std::clamp(value / burn, 0.f, 1.f);
    int fill = PAL_FLOUR;
    if (value >= z1) fill = PAL_ALERT;
    else if (value >= z0) fill = PAL_OK;
    if (fw >= 2.f) solid(x + fw - 2.f, y - 1.f, 2.f, 5.f, PAL_MARK);
    if (fw >= 1.f) solid(x, y, fw, 3.f, fill);
    float zx = x + w * (z0 / burn);
    float zw = std::max(1.f, w * ((z1 - z0) / burn));
    solid(zx, y - 1.f, zw, 5.f, PAL_GOLD);
    solid(x, y, w, 3.f, PAL_TRACK);
}

void Game::backdrop() {
    int lift = mode_ == Mode::Clear ? 2 : 0;
    int red = mode_ == Mode::Dead ? 2 : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float k = float(y) / 223.f;
        int r, g, b;
        if (k < 0.34f) {
            float u = k / 0.34f;
            r = int(2 + u * 9);
            g = int(2 + u * 3);
            b = int(5 - u * 3);
        } else {
            float u = (k - 0.34f) / 0.66f;
            r = int(11 - u * 7);
            g = int(5 - u * 3);
            b = 2;
        }
        r = std::clamp(r + lift + red, 0, 15);
        g = std::clamp(g + lift / 2, 0, 15);
        b = std::clamp(b, 0, 15);
        sys_->vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    backdrop();
    const bool title = mode_ == Mode::Title;
    const float ox = shake_ > 0.f ? std::sin(anim_ * 48.f) * shake_ * 4.f : 0.f;
    const int hot = title ? 2 : sel_;

    if (title) {
        spr(art_.title, 160, 18, float(art_.title.h), PAL_INK);
        spr(art_.sub, 160, 40, float(art_.sub.h), PAL_GOLD);
    } else if (mode_ == Mode::Clear) {
        spr(art_.winWord, 160, 22, float(art_.winWord.h), PAL_GOLD);
    } else if (mode_ == Mode::Dead) {
        spr(art_.failWord, 160, 22, float(art_.failWord.h), PAL_ALERT);
    }

    float py = kPeelY + std::sin(anim_ * 2.2f) * 1.4f;
    spr(art_.peel, slotX(hot), py, 16, PAL_WOOD);
    solid(slotX(hot) - 20.f, 108.f, 40.f, 2.f, PAL_GOLD);

    for (int i = 0; i < 6; i++) {
        float x = slotX(i) - 20.f;
        if (title) {
            meter(x, kMeterY, kDemo[i].near, kBurnNear, kTurnLo, kTurnGold);
            meter(x, kMeterY + 6.f, kDemo[i].far, kBurnFar, kPullLo, kPullGold);
        } else if (!loaf_[i].pulled) {
            meter(x, kMeterY, loaf_[i].near, kBurnNear, kTurnLo, kTurnGold);
            meter(x, kMeterY + 6.f, loaf_[i].far, kBurnFar, kPullLo, kPullGold);
        }
    }

    for (int i = 0; i < 6; i++) {
        float x = slotX(i);
        int lk = title ? kDemo[i].look : look(i);
        float y = kLoafY;
        float h = kLoafH;
        float sx = x + ox;
        if (!title && loaf_[i].pulled) {
            float u = loaf_[i].fly;
            y = kLoafY + (kCrumbY - kLoafY) * u;
            h = kLoafH + (18.f - kLoafH) * u;
            sx = x + ox * (1.f - u);
            lk = LOOK_DRAW;
        }
        bool steam = lk == LOOK_TURN || lk == LOOK_DRAW || lk == LOOK_DANGER || lk == LOOK_DARK;
        if (!title && steam && !loaf_[i].pulled) {
            float bob = std::sin(anim_ * 3.f + float(i)) * 2.f;
            spr(art_.steam, sx, y - h * 0.55f + bob, 14, PAL_FLOUR);
        }
        if (!title && loaf_[i].spark > 0.f && !loaf_[i].pulled)
            spr(art_.spark, sx, y - h * 0.72f, 8.f + loaf_[i].spark * 10.f, PAL_GOLD);
        spr(art_.loaf[lk], sx, y, h, PAL_LOAF);
    }

    for (int i = 0; i < 6; i++) {
        int frame = (int(anim_ * 8.f) + i) % 3;
        float bob = (i == hot) ? 1.f : 0.f;
        spr(art_.flame[frame], slotX(i) + ox, kFlameY - bob, kFlameH, PAL_FIRE);
        spr(art_.arch, slotX(i) + ox, kArchY, kArchH, PAL_BRICK);
    }
    spr(art_.board, 160, kBoardY, float(art_.board.h), PAL_WOOD);
    spr(art_.sun, 292, 30, float(art_.sun.h), PAL_GOLD);

    if (!title) {
        hud(1, 0, "S3 OVEN", PAL_INK);
        char right[16];
        std::snprintf(right, sizeof right, "DRAWN %d/6", drawn_);
        hud(31, 0, right, drawn_ == 6 ? PAL_OK : PAL_GOLD);
    }
    if (mode_ == Mode::Pause) {
        hudC(1, "PAUSED", PAL_GOLD);
    } else if (mode_ == Mode::Play) {
        char line[40];
        int pal = PAL_INK;
        hint(line, int(sizeof line), pal);
        hudC(1, line, pal);
    }

    for (int i = 0; i < 6; i++) {
        int cell = int(slotX(i) / 8.f);
        if (title) {
            const char* name = kDemo[i].name;
            int pal = PAL_INK;
            if (i == 2 || i == 4) pal = PAL_GOLD;
            else if (i == 5) pal = PAL_ALERT;
            hud(cell - int(std::strlen(name)) / 2, 16, name, pal);
        } else {
            const char* name = kName[i];
            hud(cell - int(std::strlen(name)) / 2, 16, name, i == sel_ ? PAL_GOLD : PAL_ASH);
            const Loaf& L = loaf_[i];
            const char* tag = " BAKE ";
            int kind = 0;
            if (L.pulled) {
                tag = "DRAWN ";
                kind = 3;
            } else if (!L.turned) {
                if (t_ < kSpec[i].delay) tag = " COLD ";
                else if (L.near < kTurnLo) tag = " BAKE ";
                else if (L.near < kTurnGold) {
                    tag = " TURN ";
                    kind = 1;
                } else {
                    tag = "HURRY!";
                    kind = 2;
                }
            } else if (L.far < kPullLo) tag = " BAKE ";
            else if (L.far < kPullGold) {
                tag = " DRAW ";
                kind = 1;
            } else {
                tag = "HURRY!";
                kind = 2;
            }
            int pal = PAL_ASH;
            if (kind == 2) pal = (int(anim_ * 8.f) & 1) ? PAL_ALERT : PAL_GOLD;
            else if (kind == 1) pal = i == sel_ ? PAL_GOLD : PAL_OK;
            else if (kind == 3) pal = PAL_OK;
            else if (i == sel_) pal = PAL_INK;
            hud(cell - 3, 18, tag, pal);
        }
    }

    if (title) {
        hudC(23, "TOP BAR TURNS, LOWER BAR DRAWS", PAL_GOLD);
        hudC(25, "BURN ONE AND THE MORNING FAILS", PAL_ALERT);
        hudC(26, "ARROWS PEEL   X TURN   C DRAW", PAL_INK);
        hudC(27, "ENTER TO BAKE", PAL_GOLD);
    } else if (mode_ == Mode::Clear) {
        hudC(25, "SIX LOAVES DRAWN   NONE BURNED", PAL_OK);
        hudC(26, "ENTER BAKES ANOTHER MORNING", PAL_INK);
    } else if (mode_ == Mode::Dead) {
        hudC(25, why_, PAL_ALERT);
        hudC(26, "THE MORNING FAILS", PAL_ALERT);
        hudC(27, "ENTER TRIES AGAIN", PAL_INK);
    } else if (mode_ == Mode::Pause) {
        hudC(25, "BURN ONE AND THE MORNING FAILS", PAL_ALERT);
        hudC(27, "ENTER RESUMES    ESC TITLE", PAL_INK);
    } else {
        hudC(25, "BURN ONE AND THE MORNING FAILS", PAL_ALERT);
        hudC(26, "ARROWS PEEL   X TURN   C DRAW", PAL_INK);
        hudC(27, "ENTER PAUSES", PAL_ASH);
    }
}

}  // namespace oven

#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace yardpouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 186.f;
constexpr float kRun = 122.f;
constexpr float kFree = 148.f;
constexpr float kDuckSp = 64.f;
constexpr float kAccel = 1100.f;
constexpr float kGrav = 860.f;
constexpr float kJumpV = -430.f;
constexpr float kWatch = 72.f;
constexpr float kSpawn = 72.f;
constexpr float kPouch0 = 150.f;
constexpr float kWin = 1568.f;
constexpr float kPark = 1460.f;
constexpr float kApproach = 1388.f;
constexpr float kWorld = 1720.f;
constexpr float kCrane = 868.f;
constexpr float kHouseX = 1610.f;

struct Pit {
    float a, b;
};
constexpr Pit kPits[2] = {{448.f, 516.f}, {1032.f, 1104.f}};

struct CutDef {
    float x, half, period, blocked, phase;
};
constexpr CutDef kCuts[2] = {
    {640.f, 32.f, 3.6f, 1.45f, 0.15f},
    {1288.f, 36.f, 4.0f, 1.55f, 0.85f},
};

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

float wrap(float t, float per) {
    float u = std::fmod(t, per);
    if (u < 0.f) u += per;
    return u;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 2;
    if (over_) return 3;
    return 1;
}

bool Game::overPit(float x) const {
    for (const Pit& p : kPits)
        if (x > p.a && x < p.b) return true;
    return false;
}

bool Game::cutOn(int i, float t) const {
    const CutDef& c = kCuts[i];
    return wrap(t + c.phase, c.period) < c.blocked;
}

float Game::cutOpen(int i, float t) const {
    const CutDef& c = kCuts[i];
    float u = wrap(t + c.phase, c.period);
    if (u < c.blocked) return 0.f;
    return c.period - u;
}

bool Game::shutterOpen(float* remain) const {
    constexpr float per = 3.35f;
    constexpr float a = 1.05f;
    constexpr float b = 2.62f;
    float u = wrap(watchT_, per);
    bool open = u >= a && u < b;
    if (remain) *remain = open ? (b - u) : 0.f;
    return open;
}

bool Game::hookDown(float t) const {
    return wrap(t + 0.4f, 2.15f) < 0.95f;
}

float Game::safeDrop(float prefer) const {
    const float pockets[] = {150.f, 360.f, 560.f, 760.f, 960.f, 1160.f, 1400.f};
    float best = 150.f;
    float bestD = 1e9f;
    for (float s : pockets) {
        if (overPit(s)) continue;
        bool bad = false;
        for (int i = 0; i < 2; i++) {
            if (cutOn(i, watchT_) && std::fabs(s - kCuts[i].x) < kCuts[i].half + 12.f) bad = true;
        }
        if (bad) continue;
        float d = std::fabs(s - prefer);
        if (d < bestD) {
            bestD = d;
            best = s;
        }
    }
    return best;
}

void Game::dump(const char* where) const {
    std::fprintf(stderr, "yardpouc %s px %.1f py %.1f vx %.1f held %d ground %d watch %.2f pouch %.1f %s\n", where,
                 px_, py_, vx_, held_ ? 1 : 0, onGround_ ? 1 : 0, watchT_, pouchX_, cause_);
}

void Game::begin() {
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    pouchX_ = kPouch0;
    held_ = false;
    onGround_ = true;
    duck_ = false;
    face_ = 1;
    fan_ = -1;
    lastSec_ = -1;
    cause_ = "";
    watchT_ = stepT_ = jumpBuf_ = stun_ = inv_ = lock_ = shake_ = 0;
    cam_ = 0;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    for (int i = 0; i < 2; i++) cutWas_[i] = cutOn(i, 0.f);
    blip(520.f, 0.05f, 0.06f);
}

void Game::finish(bool crossed, const char* why) {
    if (mode_ != Mode::Play) return;
    won_ = crossed;
    over_ = true;
    cause_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    vx_ = 0;
    vy_ = 0;
    fan_ = 0;
    fanT_ = 0;
    shake_ = crossed ? 0.3f : 0.8f;
    if (crossed) {
        sys_->rumble(0.25f, 0.7f, 220);
        sys_->setLight(40, 160, 70);
        blip(784.f, 0.07f, 0.1f);
    } else {
        sys_->rumble(0.8f, 0.3f, 200);
        sys_->setLight(170, 30, 24);
        sys_->apu.noiseBurst(0.42f, 140.f, 0.28f);
    }
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::hit(float fromX) {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.8f;
    stun_ = 0.22f;
    shake_ = 1.f;
    float away = px_ <= fromX ? -1.f : 1.f;
    vx_ = away * 150.f;
    face_ = away < 0.f ? 1 : -1;
    if (held_) {
        held_ = false;
        lock_ = 0.4f;
        pouchX_ = safeDrop(px_ + away * 16.f);
    }
    sys_->apu.noiseBurst(0.32f, 480.f, 0.12f);
    sys_->rumble(0.5f, 0.22f, 90);
    sys_->setLight(180, 40, 28);
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::fabs(vx_) > 24.f) return (int(stepT_) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (stun_ > 0.f) return;

    float goal = held_ ? (kWin + 20.f) : pouchX_;
    if (held_ && px_ > kApproach) {
        float rem = 0.f;
        bool open = shutterOpen(&rem);
        bool started = px_ > kPark + 16.f;
        if (!onGround_) {
            right = true;
        } else if (open && (rem > 1.15f || started)) {
            right = true;
            face_ = 1;
        } else if (px_ > kPark + 4.f) {
            left = true;
        } else if (px_ < kPark - 6.f) {
            right = true;
        }
        return;
    }

    bool goR = goal > px_ + 2.f;
    bool goL = goal < px_ - 2.f;
    bool handled = false;
    if (!onGround_) {
        if (goR) right = true;
        if (goL) left = true;
        handled = true;
    }
    if (!handled && goR) {
        for (const Pit& pit : kPits) {
            if (px_ >= pit.a - 24.f && px_ < pit.a - 2.f) {
                if (vx_ < 88.f && px_ > pit.a - 14.f) {
                    left = true;
                } else {
                    right = true;
                    if (vx_ > 105.f) jump = true;
                }
                handled = true;
                break;
            }
        }
    }
    if (!handled && px_ > kCrane - 46.f && px_ < kCrane + 50.f) {
        duck = true;
        if (goR) right = true;
        if (goL) left = true;
        handled = true;
    }
    if (!handled && goR) {
        for (int i = 0; i < 2; i++) {
            const CutDef& c = kCuts[i];
            float near = c.x - c.half - 28.f;
            float far = c.x + c.half + 10.f;
            if (px_ < far && px_ > near - 30.f && goal > c.x) {
                bool on = cutOn(i, watchT_);
                float op = cutOpen(i, watchT_);
                float need = (far - std::max(px_, near)) / 116.f + 0.28f;
                if (px_ < near - 2.f) {
                    right = true;
                } else if (px_ < c.x - c.half - 2.f) {
                    if (!on && op > std::max(0.95f, need)) {
                        right = true;
                    } else if (px_ > near + 3.f) {
                        left = true;
                    } else if (px_ < near - 4.f) {
                        right = true;
                    }
                } else {
                    right = true;
                }
                handled = true;
                break;
            }
        }
    }
    if (!handled) {
        if (goR) {
            right = true;
            face_ = 1;
        } else if (goL) {
            left = true;
            face_ = -1;
        }
    }
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    watchT_ += DT;
    int sec = int(watchT_);
    if (sec != lastSec_ && sec > 0 && mode_ == Mode::Play) {
        lastSec_ = sec;
        blip(sec >= int(kWatch) - 10 ? 880.f : 180.f, 0.025f, 0.03f);
    }
    for (int i = 0; i < 2; i++) {
        bool on = cutOn(i, watchT_);
        if (on && !cutWas_[i]) sys_->apu.noiseBurst(0.16f, 220.f, 0.08f);
        cutWas_[i] = on;
    }

    if (stun_ > 0.f) stun_ -= DT;
    if (inv_ > 0.f) inv_ -= DT;
    if (lock_ > 0.f) lock_ -= DT;

    duck_ = duck && stun_ <= 0.f && onGround_;
    if (stun_ <= 0.f) {
        float spd = duck_ ? kDuckSp : (held_ ? kRun : kFree);
        float target = 0.f;
        if (right && !left) target = spd;
        if (left && !right) target = -spd;
        if (right != left) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    } else {
        vx_ = approach(vx_, 0.f, 420.f * DT);
    }

    if (jump) jumpBuf_ = 0.12f;
    else jumpBuf_ = std::max(0.f, jumpBuf_ - DT);
    if (jumpBuf_ > 0.f && onGround_ && stun_ <= 0.f && !duck_) {
        vy_ = kJumpV;
        onGround_ = false;
        jumpBuf_ = 0.f;
        blip(420.f, 0.03f, 0.03f);
    }
    if (!onGround_) vy_ = std::min(520.f, vy_ + kGrav * DT);
    else vy_ = 0.f;

    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, 36.f, kWorld - 48.f);

    if (overPit(px_)) {
        onGround_ = false;
        if (py_ >= kFloor && vy_ >= 0.f && py_ > kFloor + 48.f) {
            finish(false, "GAP");
            return;
        }
    } else if (py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else {
        onGround_ = false;
    }
    if (mode_ != Mode::Play) return;

    if (onGround_ && std::fabs(vx_) > 24.f) stepT_ += std::fabs(vx_) * DT * 0.16f;

    float up = kFloor - py_;
    if (hookDown(watchT_) && std::fabs(px_ - kCrane) < 18.f && !duck_ && up < 30.f && inv_ <= 0.f) hit(kCrane);
    for (int i = 0; i < 2; i++) {
        if (cutOn(i, watchT_) && std::fabs(px_ - kCuts[i].x) < kCuts[i].half && inv_ <= 0.f) {
            hit(kCuts[i].x);
            break;
        }
    }
    if (mode_ != Mode::Play) return;

    if (held_) {
        pouchX_ = px_ + float(face_) * 12.f;
    } else if (lock_ <= 0.f && stun_ <= 0.f && onGround_ && std::fabs(px_ - pouchX_) < 22.f && up < 8.f) {
        held_ = true;
        blip(700.f, 0.07f, 0.08f);
        sys_->rumble(0.2f, 0.45f, 70);
        sys_->setLight(200, 140, 48);
    } else {
        for (int i = 0; i < 2; i++) {
            if (cutOn(i, watchT_) && std::fabs(pouchX_ - kCuts[i].x) < kCuts[i].half) {
                finish(false, "CUT");
                return;
            }
        }
        if (overPit(pouchX_)) {
            finish(false, "GAP");
            return;
        }
    }

    if (onGround_ && px_ >= kWin) {
        if (!held_) {
            finish(false, "MISSED");
            return;
        }
        float rem = 0.f;
        if (shutterOpen(&rem)) {
            finish(true, "CROSSED");
            return;
        }
        px_ = kPark;
        vx_ = -40.f;
        blip(140.f, 0.05f, 0.08f);
    }
    if (mode_ == Mode::Play && watchT_ >= kWatch) finish(false, "TIME");
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

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip, int fog, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(std::clamp(w, 1.f, 2000.f)));
    s.h = int16_t(std::lround(std::clamp(h, 1.f, 2000.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, int fog, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    stamp(m, cx, feet ? cy - h * 0.5f : cy, w, h, pal, flip, fog, shadow);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    if (!s) return;
    float width = 0.f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false, 0, false);
        x += g.w * scale + scale;
    }
}

void Game::sky(float cam) {
    gs::VDP& vdp = sys_->vdp;
    int warm = mode_ == Mode::Won ? 2 : 0;
    int dread = mode_ == Mode::Lost ? 1 : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r, g, b;
        if (y < 92) {
            float u = y / 92.f;
            r = 2 + int(4.f * u);
            g = 2 + int(2.f * u);
            b = 7 + int(u);
        } else if (y < 156) {
            float u = (y - 92) / 64.f;
            r = 6 + int(7.f * u);
            g = 4 + int(2.f * u);
            b = 8 - int(5.f * u);
        } else {
            r = 6;
            g = 4;
            b = 3;
        }
        r = std::clamp(r + warm + dread, 0, 15);
        g = std::clamp(g + warm - dread, 0, 15);
        b = std::clamp(b - dread, 0, 15);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.A.hscroll[y] = int16_t(std::lround(-cam * 0.32f));
        vdp.B.hscroll[y] = int16_t(std::lround(-cam));
        vdp.A.vscroll[y] = 0;
        vdp.B.vscroll[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = true;
    vdp.B.enabled = true;
}

void Game::drawWorld(float cam) {
    auto feet = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip = false, int fog = 0) {
        spr(m, wx - cam, foot, h, pal, flip, true, fog, false);
    };
    int fi = int(t_ * 9.f) & 1;
    int bob = int(t_ * 6.f) & 1;
    bool show = inv_ <= 0.f || (int(inv_ * 14.f) & 1) == 0;
    if (held_) {
        float bobY = std::sin(stepT_ * 0.8f) * 2.f;
        float hy = std::min(py_, kFloor + 40.f);
        spr(art_.pouch[bob], px_ + float(face_) * 14.f - cam, hy - 30.f + bobY, 20.f, PAL_POUCH, face_ < 0, false, 0,
            false);
    } else {
        feet(art_.pouch[bob], pouchX_, kFloor - 12.f, 22.f, PAL_POUCH, false, 0);
    }
    if (show) {
        float hy = std::min(py_, kFloor + 40.f);
        float lift = std::max(0.f, kFloor - py_);
        spr(art_.shadow, px_ - cam, kFloor + 3.f, std::max(4.f, 9.f - lift * 0.05f), PAL_FX, false, true, 0, true);
        feet(heroSprite(), px_, hy, 54.f, PAL_PLAYER, face_ < 0, 0);
    }
    bool open = shutterOpen(nullptr);
    bool down = hookDown(watchT_);
    float sway = std::sin(t_ * (down ? 2.1f : 1.1f)) * (down ? 5.f : 2.f);

    spr(art_.sun, 300.f - cam * 0.08f, 34.f, 22.f, PAL_FX, false, false, 0, false);
    spr(art_.tower, 150.f - cam * 0.4f, 156.f, 96.f, PAL_IRON, false, true, 8, false);
    const float farX[] = {220.f, 560.f, 980.f, 1360.f};
    for (float x : farX) spr(art_.boxcar, x - cam * 0.46f, 152.f, 40.f, PAL_CUT, x > 800.f, true, 9, false);

    float houseLeft = kHouseX - 34.f;
    float houseTop = kFloor - 100.f;
    float winX = houseLeft + 20.f;
    float winY = houseTop + 49.f;
    if (open) spr(art_.clerk, winX - cam - 8.f, winY + 2.f, 34.f, PAL_PLAYER, false, false, 0, false);
    else spr(art_.shutter, winX - cam, winY, 22.f, PAL_HOUSE, false, false, 0, false);

    feet(art_.hook, kCrane + sway, down ? (kFloor - 26.f) : (kFloor - 86.f), 26.f, PAL_IRON, false, 0);
    float hookCy = down ? (kFloor - 40.f) : (kFloor - 100.f);
    float beamCy = kFloor - 108.f;
    stamp(art_.chain, kCrane + sway - cam, (hookCy + beamCy) * 0.5f, 5.f, std::max(8.f, std::fabs(hookCy - beamCy)),
          PAL_IRON, false, 0, false);

    for (int i = 0; i < 2; i++) {
        const CutDef& c = kCuts[i];
        bool on = cutOn(i, watchT_);
        bool soon = !on && cutOpen(i, watchT_) < 0.45f;
        if (on) {
            float bobY = std::sin(t_ * 10.f + c.x) * 1.2f;
            feet(art_.boxcar, c.x, kFloor + bobY, 62.f, PAL_CUT, false, 0);
        } else {
            feet(art_.boxcar, c.x + 86.f, kFloor - 24.f, 46.f, PAL_CUT, true, 5);
        }
        feet(art_.buck, c.x - 18.f, kFloor, 44.f, PAL_WOOD, false, 0);
        spr(soon || on ? art_.lensR : art_.lensG, c.x - 18.f - cam, kFloor - 52.f, 12.f, soon || on ? PAL_ALERT : PAL_FX,
            false, false, 0, false);
    }

    feet(art_.house, kHouseX, kFloor, 100.f, PAL_HOUSE, false, 0);
    spr(art_.bell, kHouseX - cam, houseTop - 2.f, 14.f, PAL_FX, false, false, 0, false);
    feet(art_.shed, 42.f, kFloor, 68.f, PAL_WOOD, false, 0);
    feet(art_.stake, 1554.f, kFloor, 34.f, PAL_WOOD, false, 0);
    stamp(art_.plank, 120.f - cam, kFloor - 2.f, 170.f, 12.f, PAL_WOOD, false, 0, false);

    feet(art_.mast, kCrane - 48.f, kFloor, 114.f, PAL_IRON, false, 0);
    feet(art_.mast, kCrane + 48.f, kFloor, 114.f, PAL_IRON, true, 0);
    spr(art_.beam, kCrane - cam, beamCy, 16.f, PAL_IRON, false, false, 0, false);

    const float lamps[] = {250.f, 390.f, 700.f, 940.f, 1160.f, 1470.f};
    for (float x : lamps) {
        if (overPit(x)) continue;
        feet(art_.lamp, x, kFloor, 54.f, PAL_IRON, false, 0);
        spr(art_.flame[fi], x - cam, kFloor - 56.f, 12.f, PAL_FX, false, false, 0, false);
    }
    feet(art_.drum, 320.f, kFloor, 28.f, PAL_WOOD, false, 0);
    feet(art_.barrel, 378.f, kFloor, 24.f, PAL_WOOD, false, 0);
    feet(art_.crate, 900.f, kFloor, 20.f, PAL_WOOD, false, 0);
    feet(art_.crate, 1360.f, kFloor, 20.f, PAL_WOOD, true, 0);
    feet(art_.lever, 560.f, kFloor, 34.f, PAL_IRON, false, 0);
    feet(art_.lever, 1218.f, kFloor, 34.f, PAL_IRON, false, 0);
    feet(art_.chev, 580.f, kFloor - 1.f, 10.f, PAL_FX, false, 0);
    feet(art_.chev, 1224.f, kFloor - 1.f, 10.f, PAL_FX, false, 0);
    feet(art_.chev, kPark, kFloor - 1.f, 10.f, PAL_FX, false, 0);
    feet(art_.crate, kPouch0, kFloor, 18.f, PAL_WOOD, false, 0);

    for (const Pit& pit : kPits) {
        float mid = (pit.a + pit.b) * 0.5f;
        float w = (pit.b - pit.a) + 10.f;
        feet(art_.lip, pit.a, kFloor + 2.f, 16.f, PAL_EARTH, false, 0);
        feet(art_.lip, pit.b, kFloor + 2.f, 16.f, PAL_EARTH, true, 0);
        stamp(art_.water, mid - cam, kFloor + 30.f, w, 10.f, PAL_EARTH, false, 0, false);
        stamp(art_.hole, mid - cam, kFloor + 22.f, w, 54.f, PAL_EARTH, false, 0, false);
    }
}

void Game::drawTitle() {
    int fi = int(t_ * 9.f) & 1;
    int bob = int(t_ * 6.f) & 1;
    text("S3 YARD POUC", 160.f, 16.f, 1.15f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160.f, 36.f, 0.82f, PAL_FX, 0);
    text("MISS IT AND THE WATCH IS OVER", 160.f, 52.f, 0.62f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160.f, 70.f, 0.9f, PAL_HUD, 0);

    spr(art_.sun, 286.f, 28.f, 18.f, PAL_FX, false, false, 0, false);
    spr(art_.tower, 36.f, 158.f, 88.f, PAL_IRON, false, true, 7, false);
    spr(art_.boxcar, 214.f, 150.f, 36.f, PAL_CUT, false, true, 8, false);
    spr(art_.mast, 132.f, kFloor, 96.f, PAL_IRON, false, true, 0, false);
    spr(art_.mast, 196.f, kFloor, 96.f, PAL_IRON, true, true, 0, false);
    spr(art_.beam, 164.f, kFloor - 92.f, 14.f, PAL_IRON, false, false, 0, false);
    spr(art_.hook, 164.f, kFloor - 48.f, 22.f, PAL_IRON, false, false, 0, false);
    spr(art_.boxcar, 164.f, kFloor, 52.f, PAL_CUT, false, true, 0, false);
    spr(art_.buck, 118.f, kFloor, 40.f, PAL_WOOD, false, true, 0, false);
    spr(art_.lensR, 118.f, kFloor - 46.f, 11.f, PAL_ALERT, false, false, 0, false);
    spr(art_.shed, 28.f, kFloor, 58.f, PAL_WOOD, false, true, 0, false);
    spr(art_.crate, 62.f, kFloor, 16.f, PAL_WOOD, false, true, 0, false);
    spr(art_.pouch[bob], 62.f, kFloor - 14.f, 20.f, PAL_POUCH, false, true, 0, false);
    spr(art_.stand, 96.f, kFloor, 50.f, PAL_PLAYER, false, true, 0, false);
    spr(art_.shadow, 96.f, kFloor + 2.f, 8.f, PAL_FX, false, true, 0, true);
    spr(art_.house, 286.f, kFloor, 92.f, PAL_HOUSE, false, true, 0, false);
    spr(art_.clerk, 268.f, kFloor - 48.f, 30.f, PAL_PLAYER, false, false, 0, false);
    spr(art_.bell, 286.f, kFloor - 96.f, 12.f, PAL_FX, false, false, 0, false);
    spr(art_.lamp, 230.f, kFloor, 46.f, PAL_IRON, false, true, 0, false);
    spr(art_.flame[fi], 230.f, kFloor - 48.f, 10.f, PAL_FX, false, false, 0, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = cam_;
        if (shake_ > 0.f) cam += std::sin(t_ * 70.f) * shake_ * 3.f;
        cam = std::clamp(cam, 0.f, kWorld - float(gs::SCREEN_W));
    }
    sky(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160.f, 22.f, 1.05f, PAL_HUD, 0);
        text("THE WATCH HOLDS", 160.f, 44.f, 0.85f, PAL_FX, 0);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160.f, 22.f, 1.0f, PAL_ALERT, 0);
        const char* why = "THE CLOCK DIED";
        if (cause_[0] == 'G') why = "IT FELL THROUGH THE GAP";
        else if (cause_[0] == 'C' && cause_[1] == 'U') why = "THE CUT TOOK THE POUCH";
        else if (cause_[0] == 'M') why = "CROSSED WITH EMPTY HANDS";
        text(why, 160.f, 44.f, 0.7f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 28.f, 1.2f, PAL_HUD, 0);
    }
    drawWorld(cam);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - watchT_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
        hud(30, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_FX : PAL_ALERT);

        const char* hint = held_ ? "CARRY IT ACROSS" : "TAKE THE POUCH";
        int hintPal = PAL_HUD;
        if (stun_ > 0.f) {
            hint = "STEADY";
            hintPal = PAL_ALERT;
        } else if (px_ > kApproach) {
            float rem = 0.f;
            bool open = shutterOpen(&rem);
            hint = open ? "INTO THE HOUSE" : "WAIT FOR THE SHUTTER";
            hintPal = open ? PAL_FX : PAL_ALERT;
        } else if (px_ > kCrane - 50.f && px_ < kCrane + 54.f) {
            hint = "DUCK THE HOOK";
            hintPal = hookDown(watchT_) ? PAL_ALERT : PAL_HUD;
        } else {
            for (const Pit& pit : kPits) {
                if (px_ > pit.a - 70.f && px_ < pit.a) {
                    hint = "JUMP THE GAP";
                    hintPal = PAL_FX;
                }
            }
            for (int i = 0; i < 2; i++) {
                const CutDef& c = kCuts[i];
                float near = c.x - c.half - 40.f;
                float far = c.x + c.half + 8.f;
                if (px_ > near && px_ < far) {
                    bool on = cutOn(i, watchT_);
                    bool soon = !on && cutOpen(i, watchT_) < 0.45f;
                    hint = (on || soon) ? "WAIT FOR THE CUT" : "CROSS ON THE GAP";
                    hintPal = (on || soon) ? PAL_ALERT : PAL_FX;
                }
            }
        }
        hudC(1, hint, hintPal);
        hud(1, 26, "ARROWS  Z JUMP  DOWN DUCK", PAL_HUD);
    } else if (!bot_) {
        hudC(26, "START", PAL_HUD);
    }
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (fan_ >= 0) {
        static const float good[] = {262.f, 330.f, 392.f, 523.f};
        static const float bad[] = {220.f, 174.f, 146.f, 110.f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.045f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0.f;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 73.f : 55.f, 0.015f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 49.f, 0.012f);
    else sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.68f);
    t_ = 0.f;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    cam_ = 0.f;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(440.f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            sys.apu.tone(1, 0, 0);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, jump = false, duck = false;
        if (bot_) {
            bot(left, right, jump, duck);
        } else if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(280.f, 0.04f, 0.04f);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP) ||
                   pad.pressed(gs::BTN_TURBO);
            duck = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump, duck);
        float want = std::clamp(px_ - 140.f, 0.f, kWorld - float(gs::SCREEN_W));
        cam_ += (want - cam_) * std::min(1.f, DT * 8.f);
    }

    if (mode_ != Mode::Pause && shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.2f);

    if (mode_ == Mode::Won) sys.setLight(40, 160, 70);
    else if (mode_ == Mode::Lost) sys.setLight(160, 28, 18);
    else if (mode_ == Mode::Title) sys.setLight(120, 72, 32);
    else if (held_) sys.setLight(170, 110, 40);
    else sys.setLight(70, 78, 120);

    serviceAudio();
    draw();
}

}  // namespace yardpouc

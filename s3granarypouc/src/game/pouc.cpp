#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace granarypouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 186.f;
constexpr float kRun = 118.f;
constexpr float kFree = 142.f;
constexpr float kDuckSp = 62.f;
constexpr float kAccel = 1100.f;
constexpr float kGrav = 840.f;
constexpr float kJumpV = -420.f;
constexpr float kWatch = 68.f;
constexpr float kSpawn = 68.f;
constexpr float kPouch0 = 176.f;
constexpr float kWin = 1340.f;
constexpr float kPark = 1236.f;
constexpr float kApproach = 1168.f;
constexpr float kWorld = 1560.f;
constexpr float kAuger = 612.f;
constexpr float kDoorX = 980.f;
constexpr float kHatchX = 1404.f;

struct Pit {
    float a, b;
};
constexpr Pit kPits[2] = {{392.f, 468.f}, {792.f, 872.f}};

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

bool Game::doorOn(float t) const {
    constexpr float per = 4.1f;
    constexpr float blocked = 1.55f;
    constexpr float phase = 0.35f;
    return wrap(t + phase, per) < blocked;
}

float Game::doorOpen(float t) const {
    constexpr float per = 4.1f;
    constexpr float blocked = 1.55f;
    constexpr float phase = 0.35f;
    float u = wrap(t + phase, per);
    if (u < blocked) return 0.f;
    return per - u;
}

bool Game::hatchOpen(float* remain) const {
    constexpr float per = 3.4f;
    constexpr float a = 1.05f;
    constexpr float b = 2.7f;
    float u = wrap(watchT_, per);
    bool open = u >= a && u < b;
    if (remain) *remain = open ? (b - u) : 0.f;
    return open;
}

bool Game::bladeDown(float t) const { return wrap(t + 0.2f, 2.45f) < 0.82f; }

float Game::safeDrop(float prefer) const {
    const float pockets[] = {176.f, 300.f, 530.f, 700.f, 940.f, 1120.f, 1236.f};
    float best = 176.f;
    float bestD = 1e9f;
    for (float s : pockets) {
        if (overPit(s)) continue;
        if (doorOn(watchT_) && std::fabs(s - kDoorX) < 48.f) continue;
        if (bladeDown(watchT_) && std::fabs(s - kAuger) < 36.f) continue;
        float d = std::fabs(s - prefer);
        if (d < bestD) {
            bestD = d;
            best = s;
        }
    }
    return best;
}

void Game::dump(const char* where) const {
    std::fprintf(stderr, "granarypouc %s px %.1f py %.1f vx %.1f held %d ground %d watch %.2f pouch %.1f %s\n", where,
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
    doorWas_ = doorOn(0.f);
    blip(480.f, 0.05f, 0.06f);
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
    shake_ = crossed ? 0.25f : 0.8f;
    if (crossed) {
        sys_->rumble(0.25f, 0.7f, 220);
        sys_->setLight(90, 140, 40);
        blip(740.f, 0.07f, 0.1f);
    } else {
        sys_->rumble(0.8f, 0.3f, 200);
        sys_->setLight(160, 36, 18);
        sys_->apu.noiseBurst(0.42f, 140.f, 0.28f);
    }
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::hit(float fromX) {
    if (inv_ > 0.f || stun_ > 0.f || mode_ != Mode::Play) return;
    inv_ = 0.75f;
    stun_ = 0.22f;
    shake_ = 1.f;
    float away = px_ <= fromX ? -1.f : 1.f;
    vx_ = away * 150.f;
    face_ = away < 0.f ? 1 : -1;
    if (held_) {
        held_ = false;
        lock_ = 0.4f;
        pouchX_ = safeDrop(px_ + away * 18.f);
    }
    sys_->apu.noiseBurst(0.3f, 360.f, 0.12f);
    sys_->rumble(0.5f, 0.2f, 90);
    sys_->setLight(170, 50, 20);
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

    float goal = held_ ? (kWin + 24.f) : pouchX_;
    if (held_ && px_ > kApproach) {
        float rem = 0.f;
        bool open = hatchOpen(&rem);
        bool started = px_ > kPark + 16.f;
        if (!onGround_) {
            right = true;
        } else if (open && (rem > 1.2f || started)) {
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
            if (px_ >= pit.a - 28.f && px_ < pit.a - 2.f) {
                if (vx_ < 90.f && px_ > pit.a - 16.f) {
                    left = true;
                } else {
                    right = true;
                    if (vx_ > 100.f) jump = true;
                }
                handled = true;
                break;
            }
        }
    }
    if (!handled && goL) {
        for (const Pit& pit : kPits) {
            if (px_ <= pit.b + 28.f && px_ > pit.b + 2.f) {
                if (vx_ > -90.f && px_ < pit.b + 16.f) {
                    right = true;
                } else {
                    left = true;
                    if (vx_ < -100.f) jump = true;
                }
                handled = true;
                break;
            }
        }
    }
    if (!handled && px_ > kAuger - 40.f && px_ < kAuger + 44.f) {
        duck = true;
        if (goR) right = true;
        if (goL) left = true;
        handled = true;
    }
    if (!handled && goR && goal > kDoorX) {
        constexpr float half = 34.f;
        float near = kDoorX - half - 26.f;
        float far = kDoorX + half + 12.f;
        if (px_ < far && px_ > near - 36.f) {
            bool on = doorOn(watchT_);
            float op = doorOpen(watchT_);
            float need = (far - std::max(px_, near)) / 112.f + 0.3f;
            if (px_ < near - 2.f) {
                right = true;
            } else if (px_ < kDoorX - half - 2.f) {
                if (!on && op > std::max(1.05f, need)) right = true;
                else if (px_ > near + 3.f) left = true;
                else if (px_ < near - 4.f) right = true;
            } else {
                right = true;
            }
            handled = true;
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
        blip(sec >= int(kWatch) - 10 ? 860.f : 160.f, 0.02f, 0.03f);
    }
    bool door = doorOn(watchT_);
    if (door && !doorWas_) sys_->apu.noiseBurst(0.14f, 180.f, 0.07f);
    doorWas_ = door;

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
        blip(390.f, 0.03f, 0.03f);
    }
    if (!onGround_) vy_ = std::min(520.f, vy_ + kGrav * DT);
    else vy_ = 0.f;

    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, 36.f, kWorld - 48.f);

    if (overPit(px_)) {
        onGround_ = false;
        if (py_ >= kFloor && vy_ >= 0.f && py_ > kFloor + 46.f) {
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
    if (bladeDown(watchT_) && std::fabs(px_ - kAuger) < 20.f && !duck_ && up < 28.f && inv_ <= 0.f) hit(kAuger);
    if (doorOn(watchT_) && std::fabs(px_ - kDoorX) < 30.f && inv_ <= 0.f) hit(kDoorX);
    if (mode_ != Mode::Play) return;

    if (held_) {
        pouchX_ = px_ + float(face_) * 12.f;
    } else if (lock_ <= 0.f && stun_ <= 0.f && onGround_ && std::fabs(px_ - pouchX_) < 22.f && up < 8.f) {
        held_ = true;
        blip(660.f, 0.07f, 0.08f);
        sys_->rumble(0.2f, 0.4f, 70);
        sys_->setLight(190, 130, 40);
    } else {
        if (doorOn(watchT_) && std::fabs(pouchX_ - kDoorX) < 30.f) {
            finish(false, "DOOR");
            return;
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
        if (hatchOpen(&rem)) {
            finish(true, "CROSSED");
            return;
        }
        px_ = kPark;
        vx_ = -40.f;
        blip(130.f, 0.05f, 0.08f);
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

void Game::loft(float cam) {
    gs::VDP& vdp = sys_->vdp;
    int warm = mode_ == Mode::Won ? 2 : 0;
    int dread = mode_ == Mode::Lost ? 2 : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int r, g, b;
        if (y < 70) {
            r = 8;
            g = 5;
            b = 2;
        } else if (y < 150) {
            float u = (y - 70) / 80.f;
            r = 8 + int(5.f * u);
            g = 5 + int(3.f * u);
            b = 2 + int(u);
        } else {
            r = 7;
            g = 5;
            b = 2;
        }
        r = std::clamp(r + warm - dread, 0, 15);
        g = std::clamp(g + warm - dread, 0, 15);
        b = std::clamp(b - dread, 0, 15);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = uint8_t(y < 40 ? 3 : 0);
        vdp.A.hscroll[y] = int16_t(std::lround(-cam * 0.35f));
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
    int fi = int(t_ * 8.f) & 1;
    int bob = int(t_ * 5.f) & 1;
    bool show = inv_ <= 0.f || (int(inv_ * 14.f) & 1) == 0;

    const float silos[] = {40.f, 250.f, 700.f, 1100.f};
    for (float x : silos) feet(art_.silo, x, kFloor, 92.f, PAL_LOFT, x > 600.f, 6);
    feet(art_.bin, 118.f, kFloor, 36.f, PAL_WOOD, false, 2);
    feet(art_.sack, kPouch0 - 8.f, kFloor, 20.f, PAL_GRAIN, false, 0);
    feet(art_.sack, 330.f, kFloor, 22.f, PAL_GRAIN, true, 0);
    feet(art_.sack, 1088.f, kFloor, 22.f, PAL_GRAIN, false, 0);

    for (const Pit& pit : kPits) {
        float mid = (pit.a + pit.b) * 0.5f;
        float w = (pit.b - pit.a) + 8.f;
        feet(art_.lip, pit.a, kFloor + 2.f, 18.f, PAL_WOOD, false, 0);
        feet(art_.lip, pit.b, kFloor + 2.f, 18.f, PAL_WOOD, true, 0);
        stamp(art_.grain, mid - cam, kFloor + 28.f, w, 12.f, PAL_GRAIN, false, 0, false);
        stamp(art_.hole, mid - cam, kFloor + 18.f, w, 48.f, PAL_WOOD, false, 0, false);
    }

    bool down = bladeDown(watchT_);
    feet(art_.chute, kAuger - 22.f, kFloor - 8.f, 120.f, PAL_IRON, false, 0);
    feet(art_.chute, kAuger + 22.f, kFloor - 8.f, 120.f, PAL_IRON, true, 0);
    float bladeY = down ? (kFloor - 18.f) : (kFloor - 78.f);
    spr(art_.blade, kAuger - cam, bladeY, 22.f, PAL_IRON, false, false, 0, false);

    bool shut = doorOn(watchT_);
    if (shut) feet(art_.slab, kDoorX, kFloor, 70.f, PAL_DOOR, false, 0);
    else feet(art_.slab, kDoorX + 70.f, kFloor - 18.f, 48.f, PAL_DOOR, true, 4);

    bool open = hatchOpen(nullptr);
    feet(art_.hatch, kHatchX, kFloor, 108.f, PAL_LOFT, false, 0);
    float winX = kHatchX - 6.f;
    float winY = kFloor - 62.f;
    if (open) spr(art_.sack, winX - cam, winY, 16.f, PAL_GRAIN, false, false, 0, false);
    else spr(art_.slab, winX - cam, winY, 22.f, PAL_DOOR, false, false, 0, false);

    const float lamps[] = {210.f, 540.f, 900.f, 1180.f};
    for (float x : lamps) {
        if (overPit(x)) continue;
        feet(art_.lamp, x, kFloor, 52.f, PAL_IRON, false, 0);
        spr(art_.flame[fi], x - cam, kFloor - 54.f, 12.f, PAL_DUST, false, false, 0, false);
    }

    if (held_) {
        float bobY = std::sin(stepT_ * 0.8f) * 2.f;
        float hy = std::min(py_, kFloor + 36.f);
        spr(art_.pouch[bob], px_ + float(face_) * 14.f - cam, hy - 28.f + bobY, 18.f, PAL_POUCH, face_ < 0, false, 0,
            false);
    } else {
        feet(art_.pouch[bob], pouchX_, kFloor - 2.f, 20.f, PAL_POUCH, false, 0);
    }
    if (show) {
        float hy = std::min(py_, kFloor + 36.f);
        float lift = std::max(0.f, kFloor - py_);
        spr(art_.shadow, px_ - cam, kFloor + 2.f, std::max(4.f, 8.f - lift * 0.04f), PAL_DUST, false, true, 0, true);
        feet(heroSprite(), px_, hy, duck_ ? 36.f : 52.f, PAL_HAND, face_ < 0, 0);
    }
}

void Game::drawTitle() {
    int fi = int(t_ * 8.f) & 1;
    int bob = int(t_ * 5.f) & 1;
    text("S3 GRANARY POUC", 160.f, 16.f, 1.05f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160.f, 38.f, 0.78f, PAL_DUST, 0);
    text("MISS IT AND THE WATCH IS OVER", 160.f, 54.f, 0.58f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160.f, 72.f, 0.9f, PAL_HUD, 0);

    spr(art_.silo, 36.f, kFloor, 84.f, PAL_LOFT, false, true, 5, false);
    spr(art_.silo, 286.f, kFloor, 84.f, PAL_LOFT, true, true, 5, false);
    spr(art_.chute, 150.f, kFloor - 6.f, 100.f, PAL_IRON, false, true, 0, false);
    spr(art_.blade, 168.f, kFloor - 70.f, 18.f, PAL_IRON, false, false, 0, false);
    spr(art_.sack, 78.f, kFloor, 22.f, PAL_GRAIN, false, true, 0, false);
    spr(art_.pouch[bob], 92.f, kFloor - 8.f, 18.f, PAL_POUCH, false, true, 0, false);
    spr(art_.stand, 118.f, kFloor, 50.f, PAL_HAND, false, true, 0, false);
    spr(art_.shadow, 118.f, kFloor + 2.f, 8.f, PAL_DUST, false, true, 0, true);
    spr(art_.hatch, 250.f, kFloor, 96.f, PAL_LOFT, false, true, 0, false);
    spr(art_.lamp, 196.f, kFloor, 46.f, PAL_IRON, false, true, 0, false);
    spr(art_.flame[fi], 196.f, kFloor - 48.f, 10.f, PAL_DUST, false, false, 0, false);
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
    loft(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160.f, 22.f, 1.0f, PAL_HUD, 0);
        text("THE WATCH HOLDS", 160.f, 44.f, 0.8f, PAL_DUST, 0);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160.f, 22.f, 0.95f, PAL_ALERT, 0);
        const char* why = "THE CLOCK DIED";
        if (cause_[0] == 'G') why = "IT FELL INTO THE GRAIN";
        else if (cause_[0] == 'D') why = "THE BIN TOOK THE POUCH";
        else if (cause_[0] == 'M') why = "CROSSED WITH EMPTY HANDS";
        text(why, 160.f, 44.f, 0.68f, PAL_HUD, 0);
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
        hud(30, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_DUST : PAL_ALERT);

        const char* hint = held_ ? "CARRY IT ACROSS" : "TAKE THE POUCH";
        int hintPal = PAL_HUD;
        if (stun_ > 0.f) {
            hint = "STEADY";
            hintPal = PAL_ALERT;
        } else if (px_ > kApproach) {
            float rem = 0.f;
            bool open = hatchOpen(&rem);
            hint = open ? "THROUGH THE HATCH" : "WAIT FOR THE HATCH";
            hintPal = open ? PAL_DUST : PAL_ALERT;
        } else if (px_ > kAuger - 48.f && px_ < kAuger + 48.f) {
            hint = "DUCK THE AUGER";
            hintPal = bladeDown(watchT_) ? PAL_ALERT : PAL_HUD;
        } else if (std::fabs(px_ - kDoorX) < 80.f) {
            bool on = doorOn(watchT_);
            hint = on ? "WAIT FOR THE BIN" : "SLIP THE BIN";
            hintPal = on ? PAL_ALERT : PAL_DUST;
        } else {
            for (const Pit& pit : kPits) {
                if (px_ > pit.a - 80.f && px_ < pit.a) {
                    hint = "JUMP THE GRAIN";
                    hintPal = PAL_DUST;
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
        static const float good[] = {294.f, 370.f, 440.f, 587.f};
        static const float bad[] = {196.f, 164.f, 130.f, 98.f};
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
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 82.f : 55.f, 0.014f);
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
        float want = std::clamp(px_ - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
        cam_ += (want - cam_) * std::min(1.f, DT * 8.f);
    }

    if (mode_ != Mode::Pause && shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.2f);

    if (mode_ == Mode::Won) sys.setLight(90, 140, 40);
    else if (mode_ == Mode::Lost) sys.setLight(150, 30, 16);
    else if (mode_ == Mode::Title) sys.setLight(140, 90, 30);
    else if (held_) sys.setLight(180, 120, 36);
    else sys.setLight(90, 70, 30);

    serviceAudio();
    draw();
}

}  // namespace granarypouc

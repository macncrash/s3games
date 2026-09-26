#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace pouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 188.f;
constexpr float kWorld = 1800.f;
constexpr float kSpawn = 72.f;
constexpr float kPouch0 = 142.f;
constexpr float kWatch = 72.f;
constexpr float kRun = 130.f;
constexpr float kFree = 156.f;
constexpr float kDuckSp = 70.f;
constexpr float kAccel = 1100.f;
constexpr float kGrav = 880.f;
constexpr float kJumpV = -430.f;
constexpr float kBeam = 708.f;
constexpr float kApproach = 1440.f;
constexpr float kPark = 1520.f;
constexpr float kWin0 = 1612.f;
constexpr float kPost = 1644.f;

struct Span {
    float a, b;
};
constexpr Span kPits[] = {{556.f, 628.f}, {1096.f, 1176.f}};

struct Def {
    float minX, maxX, speed, endWait, hold, done, need;
};
constexpr Def kDef[3] = {
    {258.f, 398.f, 58.f, 2.20f, 204.f, 440.f, 1.50f},
    {800.f, 940.f, 54.f, 2.30f, 772.f, 990.f, 1.15f},
    {1268.f, 1384.f, 50.f, 2.50f, 1228.f, 1430.f, 1.15f},
};

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 2;
    if (over_) return 3;
    return 1;
}

bool Game::overPit(float x) const {
    for (const Span& p : kPits)
        if (x > p.a && x < p.b) return true;
    return false;
}

bool Game::barDown() const {
    float u = std::fmod(watchT_ + 0.2f, 1.8f);
    if (u < 0) u += 1.8f;
    return u < 0.70f;
}

bool Game::handsOpen(float* remain) const {
    constexpr float per = 3.0f;
    constexpr float a = 1.15f;
    constexpr float b = 2.55f;
    float u = std::fmod(watchT_, per);
    if (u < 0) u += per;
    bool open = u >= a && u < b;
    if (remain) *remain = open ? (b - u) : 0.f;
    return open;
}

void Game::dump(const char* where) const {
    std::fprintf(stderr,
                 "gatepouc %s px %.1f py %.1f vx %.1f held %d ground %d commit %d watch %.2f pouch %.1f %s\n",
                 where, px_, py_, vx_, held_ ? 1 : 0, onGround_ ? 1 : 0, commit_, watchT_, pouchX_, cause_);
    for (int i = 0; i < 3; i++) {
        const Patrol& p = patrols_[i];
        std::fprintf(stderr, "  pat %d x %.1f dir %.0f wait %.2f\n", i, p.x, p.dir, p.wait);
    }
}

void Game::begin() {
    for (int i = 0; i < 3; i++) {
        Patrol& p = patrols_[i];
        p.minX = kDef[i].minX;
        p.maxX = kDef[i].maxX;
        p.speed = kDef[i].speed;
        p.endWait = kDef[i].endWait;
        p.hold = kDef[i].hold;
        p.done = kDef[i].done;
        p.need = kDef[i].need;
        p.x = p.minX;
        p.dir = 1.f;
        p.wait = 0.f;
    }
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = vy_ = 0;
    pouchX_ = kPouch0;
    pouchVx_ = 0;
    held_ = false;
    onGround_ = true;
    duck_ = false;
    face_ = 1;
    commit_ = -1;
    fan_ = -1;
    lastSec_ = -1;
    cause_ = "";
    watchT_ = stepT_ = jumpBuf_ = stun_ = inv_ = lock_ = miss_ = shake_ = 0;
    cam_ = 0;
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
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
    shake_ = crossed ? 0.35f : 0.8f;
    if (crossed) {
        sys_->rumble(0.25f, 0.7f, 220);
        sys_->setLight(40, 170, 80);
        blip(784.f, 0.07f, 0.1f);
    } else {
        sys_->rumble(0.8f, 0.3f, 200);
        sys_->setLight(170, 30, 24);
        sys_->apu.noiseBurst(0.45f, crossed ? 400.f : 140.f, 0.3f);
    }
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::hit(float fromX) {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    inv_ = 0.85f;
    stun_ = 0.24f;
    shake_ = 1.f;
    float away = px_ <= fromX ? -1.f : 1.f;
    vx_ = away * 160.f;
    face_ = away < 0 ? 1 : -1;
    if (held_) {
        held_ = false;
        lock_ = 0.40f;
        float drop = px_ + away * 16.f;
        if (overPit(drop)) drop = px_ - away * 16.f;
        pouchX_ = drop;
        pouchVx_ = away * 50.f;
        if (overPit(pouchX_)) {
            finish(false, "DITCH");
            return;
        }
    }
    sys_->apu.noiseBurst(0.35f, 520.f, 0.12f);
    sys_->rumble(0.55f, 0.25f, 90);
    sys_->setLight(180, 40, 30);
}

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::abs(vx_) > 24.f) return (int(stepT_) & 1) ? art_.runA : art_.runB;
    return art_.stand;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    left = right = jump = duck = false;
    if (stun_ > 0) return;

    if (held_ && px_ > kApproach) {
        float rem = 0;
        bool open = handsOpen(&rem);
        if (!onGround_) {
            right = true;
            return;
        }
        // Leave the mark while the hands are open, and don't turn back mid-sprint.
        bool started = px_ > kPark + 12.f;
        if (open && (rem > 0.90f || started)) {
            right = true;
            face_ = 1;
            return;
        }
        if (px_ > kPark + 6.f) left = true;
        else if (px_ < kPark - 6.f) right = true;
        return;
    }

    float goal = held_ ? (kWin0 + 16.f) : pouchX_;
    bool goR = goal > px_ + 2.f;
    bool goL = goal < px_ - 2.f;

    if (!onGround_) {
        if (goR) right = true;
        if (goL) left = true;
        return;
    }

    if (goR) {
        for (const Span& pit : kPits) {
            if (px_ < pit.a - 30.f || px_ >= pit.a - 8.f) continue;
            if (vx_ > 120.f) {
                jump = true;
                right = true;
            } else if (px_ >= pit.a - 14.f) {
                left = true;
            } else {
                right = true;
            }
            return;
        }
    } else if (goL) {
        for (const Span& pit : kPits) {
            if (px_ > pit.b + 30.f || px_ <= pit.b + 8.f) continue;
            if (vx_ < -120.f) {
                jump = true;
                left = true;
            } else if (px_ <= pit.b + 14.f) {
                right = true;
            } else {
                left = true;
            }
            return;
        }
    }

    if (px_ > kBeam - 26.f && px_ < kBeam + 40.f) {
        duck = true;
        if (goR) right = true;
        if (goL) left = true;
        return;
    }

    for (int i = 0; i < 3; i++) {
        Patrol& p = patrols_[i];
        bool span = px_ < p.done && px_ > p.hold - 48.f && goR && goal > p.hold;
        if (commit_ == i) span = true;
        if (!span) continue;
        if (px_ >= p.done) {
            if (commit_ == i) commit_ = -1;
            continue;
        }
        bool far = p.wait > p.need && p.dir > 0.f && p.x >= p.maxX - 2.f;
        bool inHold = px_ >= p.hold - 12.f && px_ <= p.hold + 28.f;
        if (commit_ == i || (commit_ < 0 && far && inHold)) {
            commit_ = i;
            float rel = p.x - px_;
            if (rel < 58.f && rel > 38.f) {
                jump = true;
                right = true;
                face_ = 1;
                return;
            }
            if (rel <= 38.f && rel > -14.f) {
                commit_ = -1;
                left = true;
                return;
            }
            right = true;
            face_ = 1;
            return;
        }
        if (px_ > p.hold + 8.f) left = true;
        else if (px_ < p.hold - 10.f) right = true;
        return;
    }

    if (goR) {
        right = true;
        face_ = 1;
    } else if (goL) {
        left = true;
        face_ = -1;
    }
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    watchT_ += DT;
    int sec = int(watchT_);
    if (sec != lastSec_ && sec > 0 && mode_ == Mode::Play) {
        lastSec_ = sec;
        blip(sec >= int(kWatch) - 10 ? 880.f : 196.f, 0.028f, 0.03f);
    }

    for (Patrol& p : patrols_) {
        if (p.wait > 0) {
            p.wait -= DT;
            if (p.wait <= 0) {
                p.wait = 0;
                p.dir = -p.dir;
            }
            continue;
        }
        p.x += p.dir * p.speed * DT;
        if (p.dir > 0 && p.x >= p.maxX) {
            p.x = p.maxX;
            p.wait = p.endWait;
        } else if (p.dir < 0 && p.x <= p.minX) {
            p.x = p.minX;
            p.wait = p.endWait * 0.70f;
        }
    }

    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;
    if (lock_ > 0) lock_ -= DT;
    if (miss_ > 0) miss_ -= DT;

    duck_ = duck && stun_ <= 0 && onGround_;
    if (stun_ <= 0) {
        float spd = duck_ ? kDuckSp : (held_ ? kRun : kFree);
        float target = 0;
        if (right && !left) target = spd;
        if (left && !right) target = -spd;
        if (right != left) face_ = right ? 1 : -1;
        vx_ = approach(vx_, target, kAccel * DT);
    } else {
        vx_ = approach(vx_, 0.f, 420.f * DT);
    }

    if (jump) jumpBuf_ = 0.12f;
    else jumpBuf_ = std::max(0.f, jumpBuf_ - DT);
    if (jumpBuf_ > 0 && onGround_ && stun_ <= 0 && !duck_) {
        vy_ = kJumpV;
        onGround_ = false;
        jumpBuf_ = 0;
        blip(420.f, 0.03f, 0.03f);
    }

    if (!onGround_) vy_ = std::min(520.f, vy_ + kGrav * DT);
    else vy_ = 0;

    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, 40.f, kWorld - 64.f);

    if (onGround_ && std::abs(vx_) > 24.f) stepT_ += std::abs(vx_) * DT * 0.18f;

    if (overPit(px_)) {
        onGround_ = false;
        if (py_ >= kFloor && vy_ >= 0 && py_ > kFloor + 52.f) {
            finish(false, "DITCH");
            return;
        }
    } else if (py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0;
        onGround_ = true;
    } else {
        onGround_ = false;
    }

    if (mode_ != Mode::Play) return;

    float up = kFloor - py_;
    if (barDown() && std::abs(px_ - kBeam) < 16.f && up < 68.f && up + bodyH() > 32.f && inv_ <= 0) {
        hit(kBeam + 4.f);
        if (mode_ != Mode::Play) return;
        px_ = std::min(px_, kBeam - 22.f);
    }

    if (mode_ == Mode::Play && inv_ <= 0 && up < 42.f) {
        for (const Patrol& p : patrols_) {
            if (std::abs(p.x - px_) < 15.f) {
                hit(p.x);
                break;
            }
        }
    }
    if (mode_ != Mode::Play) return;

    if (held_) {
        pouchX_ = px_ + face_ * 10.f;
        pouchVx_ = 0;
    } else {
        pouchX_ += pouchVx_ * DT;
        pouchVx_ = approach(pouchVx_, 0.f, 240.f * DT);
        pouchX_ = std::clamp(pouchX_, 48.f, kWorld - 80.f);
        if (overPit(pouchX_)) {
            finish(false, "DITCH");
            return;
        }
        if (lock_ <= 0 && stun_ <= 0 && onGround_ && std::abs(px_ - pouchX_) < 22.f && up < 8.f) {
            held_ = true;
            blip(700.f, 0.07f, 0.08f);
            sys_->rumble(0.2f, 0.45f, 70);
            sys_->setLight(200, 140, 50);
        }
    }

    if (mode_ == Mode::Play && onGround_ && px_ >= kWin0 && miss_ <= 0) {
        float rem = 0;
        if (held_ && handsOpen(&rem)) {
            finish(true, "CROSSED");
            return;
        }
        miss_ = 0.45f;
        px_ = kPark;
        vx_ = -70.f;
        blip(140.f, 0.05f, 0.08f);
    }

    if (mode_ == Mode::Play && watchT_ >= kWatch) finish(false, "TIME");
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    float width = 0;
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
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false, false);
        x += g.w * scale + scale;
    }
}

void Game::sky(float cam) {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 96) {
            float u = y / 96.f;
            c = gs::rgb4(1 + int(2 * u), 1 + int(u), 5 + int(4 * u));
        } else if (y < 156) {
            float u = (y - 96) / 60.f;
            c = gs::rgb4(3 + int(6 * u), 2 + int(3 * u), 8 - int(4 * u));
        } else {
            c = gs::rgb4(3, 2, 2);
        }
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = 0;
        vdp.A.hscroll[y] = int16_t(std::lround(-cam * 0.28f));
        vdp.B.hscroll[y] = int16_t(std::lround(-cam));
        vdp.A.vscroll[y] = 0;
        vdp.B.vscroll[y] = 0;
        vdp.road[y].on = false;
    }
}

void Game::drawWorld(float cam) {
    auto world = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip, bool feet = true) {
        spr(m, wx - cam, foot, h, pal, flip, feet, false);
    };
    int fi = int(t_ * 10.f) & 1;
    int bob = int(t_ * 6.f) & 1;

    bool show = inv_ <= 0 || (int(inv_ * 16.f) & 1) == 0;
    if (show && mode_ != Mode::Title) {
        float hy = py_ > kFloor ? kFloor : py_;
        spr(art_.shadow, px_ - cam, kFloor + 2.f, 8, PAL_FX, false, true, true);
        spr(heroSprite(), px_ - cam, hy, 54, PAL_PLAYER, face_ < 0, true, false);
    }
    if (held_) {
        float bobY = std::sin(stepT_ * 0.7f) * 2.f;
        world(art_.pouch[bob], px_ + face_ * 14.f, (py_ > kFloor ? kFloor : py_) - 34.f + bobY, 22, PAL_POUCH,
              face_ < 0, false);
    } else {
        world(art_.pouch[bob], pouchX_, kFloor - 2.f, 22, PAL_POUCH, false);
        world(art_.stool, kPouch0, kFloor, 16, PAL_WOOD, false);
    }

    for (const Patrol& p : patrols_) {
        int pose = 0;
        if (p.wait <= 0) pose = (int(t_ * 6.f + p.minX) & 1) ? 1 : 2;
        world(art_.patrol[pose], p.x, kFloor, 58, PAL_PATROL, p.dir < 0);
        spr(art_.shadow, p.x - cam, kFloor + 2.f, 8, PAL_FX, false, true, true);
    }

    bool open = handsOpen(nullptr);
    world(art_.post, kPost, kFloor, 92, PAL_WOOD, false);
    world(art_.bell, kPost, kFloor - 96.f, 16, PAL_POUCH, false, false);
    world(open ? art_.guardOpen : art_.guardShut, kPost - 6.f, kFloor, 60, PAL_POST, false);

    float barY = barDown() ? (kFloor - 46.f) : (kFloor - 96.f);
    world(art_.bar, kBeam, barY, 16, PAL_STONE, false, false);
    world(art_.beamPost, kBeam - 34.f, kFloor, 78, PAL_WOOD, false);
    world(art_.beamPost, kBeam + 34.f, kFloor, 78, PAL_WOOD, false);

    for (const Span& pit : kPits) {
        world(art_.lip, pit.a, kFloor + 2.f, 18, PAL_EARTH, false);
        world(art_.lip, pit.b, kFloor + 2.f, 18, PAL_EARTH, true);
        float mid = (pit.a + pit.b) * 0.5f;
        float w = (pit.b - pit.a) + 12.f;
        float h = 58.f;
        gs::Sprite s;
        s.w = int16_t(std::lround(w));
        s.h = int16_t(std::lround(h));
        s.x = int16_t(std::lround(mid - cam - w * 0.5f));
        s.y = 172;
        s.img = art_.hole.pick(h);
        s.pal = PAL_STONE;
        sys_->vdp.sprite(s);
    }

    const float lamps[] = {236.f, 470.f, 860.f, 1210.f, 1490.f};
    for (float x : lamps) {
        if (overPit(x)) continue;
        world(art_.flame[fi], x, kFloor - 40.f, 12, PAL_FX, false, false);
        world(art_.lamp, x, kFloor, 40, PAL_WOOD, false);
    }

    world(art_.door, 108.f, kFloor, 62, PAL_WOOD, false);
    world(art_.arch, 108.f, 118.f, 36, PAL_STONE, false, false);
    world(art_.tower, 52.f, kFloor, 124, PAL_STONE, false);
    world(art_.tower, 164.f, kFloor, 124, PAL_STONE, true);
    world(art_.flame[fi], 196.f, kFloor - 34.f, 12, PAL_FX, false, false);
    world(art_.lamp, 196.f, kFloor, 36, PAL_WOOD, false);

    for (int i = 0; i < 30; i++) {
        float x = 30.f + i * 58.f;
        if (overPit(x) || std::abs(x - kBeam) < 40.f) continue;
        world(art_.stone, x, kFloor + 1.f, 8, PAL_STONE, false);
    }
    spr(art_.moon, 246.f - cam * 0.08f, 34.f, 22, PAL_FX, false, false, false);
}

void Game::drawTitle() {
    int fi = int(t_ * 10.f) & 1;
    int bob = int(t_ * 6.f) & 1;
    text("S3 GATE POUC", 160, 14, 1.25f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160, 36, 1.0f, PAL_FX, 0);
    text("MISS IT AND THE WATCH IS OVER", 160, 54, 0.85f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160, 76, 1.0f, PAL_HUD, 0);

    spr(art_.moon, 262, 28, 20, PAL_FX, false, false, false);
    spr(art_.tower, 46, kFloor, 118, PAL_STONE, false, true, false);
    spr(art_.tower, 150, kFloor, 118, PAL_STONE, true, true, false);
    spr(art_.arch, 98, 112, 32, PAL_STONE, false, false, false);
    spr(art_.door, 98, kFloor, 58, PAL_WOOD, false, true, false);
    spr(art_.stand, 196, kFloor, 52, PAL_PLAYER, false, true, false);
    spr(art_.shadow, 196, kFloor + 2, 8, PAL_FX, false, true, true);
    spr(art_.stool, 246, kFloor, 16, PAL_WOOD, false, true, false);
    spr(art_.pouch[bob], 246, kFloor - 8, 24, PAL_POUCH, false, true, false);
    spr(art_.flame[fi], 168, kFloor - 32, 11, PAL_FX, false, false, false);
    spr(art_.lamp, 168, kFloor, 34, PAL_WOOD, false, true, false);
    spr(art_.guardOpen, 292, kFloor, 52, PAL_POST, false, true, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0;
    if (mode_ != Mode::Title) {
        cam = cam_;
        if (shake_ > 0) cam += std::sin(t_ * 70.f) * shake_ * 3.f;
        cam = std::clamp(cam, 0.f, kWorld - gs::SCREEN_W);
    }
    sky(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160, 28, 1.15f, PAL_HUD, 0);
        text("THE WATCH HOLDS", 160, 50, 1.0f, PAL_FX, 0);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160, 28, 1.15f, PAL_ALERT, 0);
        text(cause_[0] == 'D' ? "THE POUCH MISSED THE ROAD" : "THE CLOCK DIED", 160, 50, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 36, 1.4f, PAL_HUD, 0);
    }
    drawWorld(cam);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - watchT_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
        hud(32, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_FX : PAL_ALERT);
        const char* hint = "TAKE THE POUCH";
        if (held_ && px_ >= kApproach) {
            float rem = 0;
            hint = handsOpen(&rem) ? "INTO THE HANDS" : "WAIT FOR THE HANDS";
        } else if (held_) {
            hint = "CARRY IT ACROSS";
        }
        hudC(1, hint, PAL_HUD);
        hud(1, 26, "ARROWS  Z JUMP  DOWN DUCK", PAL_HUD);
    } else if (!bot_) {
        hudC(26, "START", PAL_HUD);
    }
}

void Game::serviceAudio() {
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys_->apu.tone(0, 0, 0);
    }
    if (fan_ >= 0) {
        static const float good[] = {392.f, 523.f, 659.f, 784.f};
        static const float bad[] = {220.f, 174.f, 146.f, 110.f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.045f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 65.f : 49.f, 0.016f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 55.f, 0.012f);
    else sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.68f);
    t_ = 0;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    cam_ = 0;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
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
        } else {
            if (pad.pressed(gs::BTN_START)) {
                mode_ = Mode::Pause;
                blip(280.f, 0.04f, 0.04f);
            } else {
                left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
                right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
                jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP) ||
                       pad.pressed(gs::BTN_TURBO);
                duck = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
            }
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump, duck);
        float want = std::clamp(px_ - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
        cam_ += (want - cam_) * std::min(1.f, DT * 7.f);
        if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT * 2.4f);
    }

    if (mode_ == Mode::Play && held_) sys.setLight(170, 110, 40);
    else if (mode_ == Mode::Play) sys.setLight(70, 80, 120);
    else if (mode_ == Mode::Title) sys.setLight(50, 60, 110);

    serviceAudio();
    draw();
}

}  // namespace pouc

#include "game/golf.h"

#include "game/course.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <string>

namespace golf {
namespace {

constexpr int kTitle = 0, kPlay = 1, kPause = 2, kBanner = 3, kWin = 4, kLose = 5;
constexpr float kDt = 1.f / 240.f;
constexpr float kG = 500.f;
constexpr float kR = 3.6f;
constexpr float kCapture = 96.f;
constexpr float kPi = 3.14159265f;
constexpr int kSub = 4;
constexpr int kMaxShot = 420;
constexpr int kPickup = 10;

enum { CLUB_DRIVER, CLUB_IRON, CLUB_WEDGE, CLUB_PUTT, CLUB_N };

struct Club {
    const char* name;
    float amin, amax, vmin, vmax;
    bool putt;
};

constexpr Club kClub[CLUB_N] = {
    {"DRIVER", 16.f, 32.f, 220.f, 500.f, false},
    {"IRON", 28.f, 46.f, 170.f, 430.f, false},
    {"WEDGE", 40.f, 60.f, 140.f, 460.f, false},
    {"PUTTER", 0.f, 0.f, 24.f, 160.f, true},
};

float frictionOf(int kind) {
    switch (kind) {
    case KIND_GREEN: return 46.f;
    case KIND_SAND: return 280.f;
    case KIND_ROUGH: return 160.f;
    default: return 88.f;
    }
}

float bounceOf(int kind) {
    switch (kind) {
    case KIND_GREEN: return 0.14f;
    case KIND_SAND: return 0.04f;
    case KIND_ROUGH: return 0.08f;
    default: return 0.2f;
    }
}

float retainOf(int kind, int club) {
    float base = 0.72f;
    if (kind == KIND_GREEN) base = 0.5f;
    else if (kind == KIND_SAND) base = 0.22f;
    else if (kind == KIND_ROUGH) base = 0.4f;
    float spin = 1.f;
    if (club == CLUB_WEDGE) spin = 0.5f;
    else if (club == CLUB_IRON) spin = 0.78f;
    return base * spin;
}

const char* kindName(int kind, bool holed) {
    if (holed) return "IN THE HOLE";
    switch (kind) {
    case KIND_GREEN: return "ON THE GREEN";
    case KIND_SAND: return "IN THE SAND";
    case KIND_ROUGH: return "IN THE ROUGH";
    case KIND_WATER: return "WET";
    case KIND_CUP: return "IN THE HOLE";
    default: return "FAIRWAY";
    }
}

}  // namespace

void Game::place(Ball& b, float x) const {
    const Hole& h = holeAt(hole_);
    b = Ball{};
    b.x = std::clamp(x, 2.f, h.length - 2.f);
    Surf s = groundAt(h, b.x, false);
    if (s.kind == KIND_WATER) s = groundAt(h, h.tee, false);
    b.y = s.y + kR;
    b.prevX = b.x;
    b.prevY = b.y;
    b.kind = s.kind;
    b.rest = true;
}

void Game::launch(Ball& b, int club, float ang, float spd) const {
    const Hole& h = holeAt(hole_);
    b.holed = false;
    b.rest = false;
    b.wet = false;
    b.ob = false;
    b.inWell = false;
    b.skipped = false;
    b.rolling = false;
    b.prevX = b.x;
    b.prevY = b.y;
    if (club < 0 || club >= CLUB_N) club = CLUB_WEDGE;
    if (kClub[club].putt) {
        float dir = std::cos(ang) >= 0.f ? 1.f : -1.f;
        b.vx = dir * std::fabs(spd);
        b.vy = 0;
        b.rolling = true;
        Surf s = groundAt(h, b.x, false);
        b.y = s.y + kR;
        b.kind = s.kind;
        return;
    }
    b.vx = std::cos(ang) * spd;
    b.vy = std::sin(ang) * spd;
}

void Game::tick(Ball& b, int club, float dt) const {
    if (b.rest || b.holed || b.wet || b.ob) return;
    const Hole& h = holeAt(hole_);
    b.prevX = b.x;
    b.prevY = b.y;

    if (!b.rolling) {
        b.vy -= kG * dt;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        if (b.x < 1.5f || b.x > h.length - 1.5f || b.y < -60.f) {
            b.ob = true;
            b.rest = true;
            b.vx = b.vy = 0;
            b.rolling = false;
            return;
        }
        if (waterSpan(h, b.x) && b.y <= kWaterY + kR) {
            b.wet = true;
            b.rest = true;
            b.vx = b.vy = 0;
            b.rolling = false;
            return;
        }
        bool over = std::fabs(b.x - h.cup) <= h.cupHalf;
        if (over) {
            if (b.y < h.lip) b.inWell = true;
            if (b.inWell) {
                float innerL = h.cup - h.cupHalf + kR;
                float innerR = h.cup + h.cupHalf - kR;
                if (innerR < innerL) {
                    innerL = h.cup;
                    innerR = h.cup;
                }
                if (b.x < innerL) {
                    b.x = innerL;
                    b.vx = std::fabs(b.vx) * 0.2f;
                }
                if (b.x > innerR) {
                    b.x = innerR;
                    b.vx = -std::fabs(b.vx) * 0.2f;
                }
            }
            Surf floor = groundAt(h, b.x, true);
            if (b.y <= floor.y + kR && b.y < h.lip) {
                b.y = floor.y + kR;
                b.vx = b.vy = 0;
                b.holed = true;
                b.rest = true;
                b.rolling = false;
                b.inWell = true;
                b.kind = KIND_CUP;
            }
            return;
        }
        Surf s = groundAt(h, b.x, false);
        if (s.kind == KIND_WATER) return;
        if (b.y <= s.y + kR) {
            b.y = s.y + kR;
            b.kind = s.kind;
            float incoming = b.vy;
            b.vy = -incoming * bounceOf(s.kind);
            b.vx *= retainOf(s.kind, club);
            if (b.vy > 420.f) b.vy = 420.f;
            if (std::fabs(b.vy) < 40.f || incoming > -8.f) {
                b.vy = 0;
                b.rolling = true;
            }
        }
        return;
    }

    bool over = std::fabs(b.x - h.cup) <= h.cupHalf - 0.2f;
    if (over && std::fabs(b.vx) <= kCapture) {
        b.rolling = false;
        b.inWell = true;
        b.vy = -40.f;
        return;
    }
    if (over && std::fabs(b.vx) > kCapture) b.skipped = true;

    Surf here = groundAt(h, b.x, false);
    Surf left = groundAt(h, b.x - 4.f, false);
    Surf right = groundAt(h, b.x + 4.f, false);
    float slope = (right.y - left.y) / 8.f;
    float nrm = std::sqrt(1.f + slope * slope);
    float ax = -kG * slope / nrm;
    float fr = frictionOf(here.kind);
    if (std::fabs(b.vx) < 7.f && std::fabs(ax) <= fr + 1.f) {
        b.vx = b.vy = 0;
        b.rest = true;
        b.rolling = false;
        b.y = here.y + kR;
        b.kind = here.kind;
        return;
    }
    if (std::fabs(b.vx) >= 1.f) ax -= std::copysign(fr, b.vx);
    else ax -= std::copysign(fr, ax == 0.f ? 1.f : ax);
    b.vx += ax * dt;
    if (b.vx > 640.f) b.vx = 640.f;
    if (b.vx < -640.f) b.vx = -640.f;
    b.x += b.vx * dt;
    if (b.x < 1.5f || b.x > h.length - 1.5f) {
        b.ob = true;
        b.rest = true;
        b.rolling = false;
        b.vx = b.vy = 0;
        return;
    }
    Surf n = groundAt(h, b.x, false);
    if (n.kind == KIND_WATER || (waterSpan(h, b.x) && n.y + kR < b.y - 2.f)) {
        b.rolling = false;
        b.vy = -10.f;
        return;
    }
    float ny = n.y + kR;
    if (ny < b.y - 4.f) {
        b.rolling = false;
        b.vy = 0;
        return;
    }
    if (ny > b.y + 6.f) {
        b.x = b.prevX;
        b.vx = -b.vx * 0.2f;
        b.y = here.y + kR;
        return;
    }
    b.y = ny;
    b.vy = 0;
    b.kind = n.kind;
}

bool Game::belowLip(const Ball& b) const {
    const Hole& h = holeAt(hole_);
    return b.holed && b.inWell && b.y < h.lip - 4.f;
}

Game::SimOut Game::simulate(const Ball& from, int club, float ang, float spd) const {
    Ball b = from;
    b.rest = true;
    launch(b, club, ang, spd);
    for (int f = 0; f < kMaxShot && !b.rest && !b.holed && !b.wet && !b.ob; f++) {
        for (int s = 0; s < kSub; s++) tick(b, club, kDt);
    }
    if (!b.holed && !b.wet && !b.ob) {
        b.vx = b.vy = 0;
        b.rolling = false;
        b.rest = true;
        if (!b.inWell) {
            Surf s = groundAt(holeAt(hole_), b.x, false);
            b.y = s.y + kR;
            b.kind = s.kind;
        }
    }
    const Hole& h = holeAt(hole_);
    Game::SimOut o;
    o.holed = belowLip(b);
    o.wet = b.wet;
    o.ob = b.ob;
    o.x = b.x;
    o.kind = b.kind;
    o.onGreen = !o.holed && !o.wet && !o.ob && b.kind == KIND_GREEN && b.y > h.lip - 1.f;
    o.putt = o.onGreen && puttLine(h, b.x);
    return o;
}

float Game::crossX(const Ball& from, int club, float ang, float spd) const {
    const Hole& h = holeAt(hole_);
    Ball b = from;
    launch(b, club, ang, spd);
    bool climbed = false;
    float watch = h.lip;
    for (int f = 0; f < 240; f++) {
        for (int s = 0; s < kSub; s++) {
            float py = b.y;
            tick(b, club, kDt);
            if (b.y > watch + kR + 6.f) climbed = true;
            if (b.holed) return h.cup;
            if (b.wet || b.ob) return b.x;
            if (climbed && py >= watch && b.y < watch) return b.x;
            if (b.rolling || (b.rest && !b.holed)) return b.x;
        }
    }
    return b.x;
}

float Game::bisect(const Ball& from, int club, float ang, float target) const {
    const Club& c = kClub[club];
    bool right = std::cos(ang) >= 0.f;
    float lo = c.vmin;
    float hi = c.vmax;
    for (int i = 0; i < 16; i++) {
        float mid = 0.5f * (lo + hi);
        float x = crossX(from, club, ang, mid);
        if (right) {
            if (x < target) lo = mid;
            else hi = mid;
        } else if (x > target) lo = mid;
        else hi = mid;
    }
    return 0.5f * (lo + hi);
}

Game::Shot Game::plan() const {
    const Hole& h = holeAt(hole_);
    const Ball from = ball_;
    auto tryHole = [&](int club, float ang, float spd, Shot& out) {
        spd = std::clamp(spd, kClub[club].vmin, kClub[club].vmax);
        SimOut r = simulate(from, club, ang, spd);
        if (!r.holed) return false;
        out.club = club;
        out.ang = ang;
        out.spd = spd;
        return true;
    };

    Shot hole{};
    if (puttLine(h, from.x)) {
        float ang = h.cup >= from.x ? 0.f : kPi;
        for (float spd = kClub[CLUB_PUTT].vmin; spd <= kClub[CLUB_PUTT].vmax; spd += 2.f) {
            if (tryHole(CLUB_PUTT, ang, spd, hole)) return hole;
        }
    }

    const float nudge[] = {0, -8, 8, -18, 18, -32, 32, -50, 50};
    for (int club : {CLUB_WEDGE, CLUB_IRON, CLUB_DRIVER}) {
        const Club& c = kClub[club];
        for (float deg = c.amin; deg <= c.amax + 0.1f; deg += 3.f) {
            float ang = deg * kPi / 180.f;
            if (h.cup < from.x - 6.f) ang = kPi - ang;
            float spd = bisect(from, club, ang, h.cup);
            for (float d : nudge) {
                if (tryHole(club, ang, spd + d, hole)) return hole;
            }
        }
    }

    float target = h.cup - h.cupHalf - 20.f;
    if (!puttLine(h, target)) {
        target = h.cup;
        for (float d = 14.f; d < 140.f; d += 4.f) {
            if (puttLine(h, h.cup - d)) {
                target = h.cup - d;
                break;
            }
            if (puttLine(h, h.cup + d)) {
                target = h.cup + d;
                break;
            }
        }
    }

    Shot best{CLUB_WEDGE, 48.f * kPi / 180.f, 280.f};
    float bestCost = 1e9f;
    auto score = [&](const SimOut& r) {
        if (r.holed) return -10000.f;
        float c = std::fabs(r.x - target);
        if (r.wet || r.ob) c += 5000.f;
        if (r.kind == KIND_SAND) c += 350.f;
        if (r.putt) c -= 320.f;
        else if (r.onGreen) c -= 40.f;
        return c;
    };
    const float layNudge[] = {0, -12, 12, -28, 28, -48, 48};
    for (int club : {CLUB_WEDGE, CLUB_IRON, CLUB_DRIVER}) {
        const Club& c = kClub[club];
        for (float deg = c.amin; deg <= c.amax + 0.1f; deg += 4.f) {
            float ang = deg * kPi / 180.f;
            if (target < from.x) ang = kPi - ang;
            float spd = bisect(from, club, ang, target);
            for (float d : layNudge) {
                float s = std::clamp(spd + d, c.vmin, c.vmax);
                SimOut r = simulate(from, club, ang, s);
                float c = score(r);
                if (r.holed) return Shot{club, ang, s};
                if (c < bestCost) {
                    bestCost = c;
                    best = Shot{club, ang, s};
                }
            }
        }
    }
    return best;
}

float Game::shotAngle(int club, float aim) const {
    if (club < 0 || club >= CLUB_N) club = CLUB_WEDGE;
    if (kClub[club].putt) return std::cos(aim) >= 0.f ? 0.f : kPi;
    float dir = std::cos(aim) >= 0.f ? 1.f : -1.f;
    float deg = std::atan2(std::sin(aim), std::fabs(std::cos(aim))) * (180.f / kPi);
    deg = std::clamp(deg, kClub[club].amin, kClub[club].amax);
    float rad = deg * kPi / 180.f;
    return dir < 0.f ? (kPi - rad) : rad;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    sys.apu.setMaster(0.65f);
    if (!sys.headless) {
        std::string saved = sys.loadBlob("s3golf-best");
        best_ = saved.empty() ? 0 : std::atoi(saved.c_str());
    }
    hole_ = 0;
    loadHole();
    if (bot_) startRound();
    else mode_ = kTitle;
}

void Game::startRound() {
    hole_ = 0;
    cups_ = 0;
    strokes_ = 0;
    won_ = false;
    over_ = false;
    bannerT_ = 0;
    sayT_ = 0;
    loadHole();
    mode_ = kPlay;
}

void Game::loadHole() {
    const Hole& h = holeAt(hole_);
    place(ball_, h.tee);
    lieX_ = h.tee;
    lie_ = "TEE";
    holeStrokes_ = 0;
    charging_ = false;
    meter_ = 0;
    meterDir_ = 1;
    shotFrames_ = 0;
    swingT_ = 0;
    needSettle_ = false;
    club_ = CLUB_WEDGE;
    flyClub_ = CLUB_WEDGE;
    aim_ = 40.f * kPi / 180.f;
    sayT_ = 0;
}

void Game::swing(int club, float ang, float spd) {
    if (club < 0 || club >= CLUB_N) club = CLUB_WEDGE;
    flyClub_ = club;
    club_ = club;
    aim_ = ang;
    launch(ball_, club, ang, spd);
    strokes_++;
    holeStrokes_++;
    charging_ = false;
    shotFrames_ = 0;
    swingT_ = 8;
    needSettle_ = true;
    sayT_ = 0;
    if (sys_) sys_->apu.noiseBurst(0.22f, 1800.f, 0.05f);
}

void Game::holeOut() {
    if (!belowLip(ball_)) return;
    const Hole& h = holeAt(hole_);
    ball_.x = h.cup;
    ball_.y = (h.lip - h.cupDepth) + kR;
    ball_.vx = ball_.vy = 0;
    ball_.rest = true;
    ball_.holed = true;
    ball_.inWell = true;
    ball_.kind = KIND_CUP;
    lie_ = "IN THE HOLE";
    cups_++;
    say("IN THE HOLE", PAL_GREEN, 1.2f);
    chime_ = 3;
    chimeT_ = 0;
    if (sys_ && !sys_->headless) sys_->rumble(0.2f, 0.55f, 80);
    if (cups_ >= kHoles) {
        mode_ = kWin;
        won_ = true;
        over_ = true;
        if (!bot_ && sys_ && !sys_->headless && (best_ == 0 || strokes_ < best_)) {
            best_ = strokes_;
            sys_->saveBlob("s3golf-best", std::to_string(best_));
        }
    } else {
        mode_ = kBanner;
        bannerT_ = 0;
    }
}

void Game::replay(const char* msg, int pal) {
    place(ball_, lieX_);
    lie_ = msg;
    say(msg, pal, 0.9f);
    charging_ = false;
    needSettle_ = true;
    if (sys_) sys_->apu.noiseBurst(0.35f, 220.f, 0.16f);
}

void Game::settle() {
    if (ball_.kind == KIND_GREEN && puttLine(holeAt(hole_), ball_.x)) {
        club_ = CLUB_PUTT;
        aim_ = holeAt(hole_).cup >= ball_.x ? 0.f : kPi;
        lie_ = "ON THE GREEN";
        say("ON THE GREEN", PAL_RED, 1.1f);
        if (sys_) sys_->apu.tone(1, 196.f, 0.06f);
        toneKill_ = std::max(toneKill_, 0.12f);
    } else {
        if (club_ == CLUB_PUTT) club_ = CLUB_WEDGE;
        float dir = holeAt(hole_).cup >= ball_.x ? 1.f : -1.f;
        float loft = 40.f * kPi / 180.f;
        aim_ = dir > 0.f ? loft : (kPi - loft);
        lie_ = kindName(ball_.kind, false);
    }
    needSettle_ = false;
}

void Game::say(const char* s, int pal, float time) {
    say_ = s ? s : "";
    sayPal_ = pal;
    sayT_ = time;
}

void Game::chime(float dt) {
    if (!sys_) return;
    if (chime_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0) {
            const float notes[3] = {523.25f, 659.25f, 783.99f};
            sys_->apu.tone(0, notes[3 - chime_], 0.09f);
            chimeT_ = 0.09f;
            chime_--;
            if (chime_ == 0) toneKill_ = 0.18f;
        }
    }
    if (toneKill_ > 0) {
        toneKill_ -= dt;
        if (toneKill_ <= 0) {
            sys_->apu.tone(0, 0, 0);
            sys_->apu.tone(1, 0, 0);
        }
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (!sys_ || h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, int(m.h)));
    gs::Sprite s;
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 400));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        float u = std::clamp(y / 150.f, 0.f, 1.f);
        int r = 4 + int(u * 8);
        int g = 6 + int(u * 6);
        int b = 12 - int(u * 2);
        if (y > 150) {
            r = 12;
            g = 10;
            b = 7;
        }
        v.lineBackdrop[y] = gs::rgb4(r, g, b);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    const Hole& h = holeAt(hole_);

    float cam = ball_.x - 108.f;
    if (mode_ == kTitle) cam = 0.f;
    cam = std::clamp(cam, 0.f, std::max(0.f, h.length - 320.f));
    auto worldY = [&](float x) { return kGround0 - groundAt(h, x, false).y; };

    bool showAim = ball_.rest && !ball_.holed && (mode_ == kPlay || mode_ == kTitle || mode_ == kPause);
    float dotX[14]{}, dotY[14]{};
    int dots = 0;
    if (showAim) {
        int club = mode_ == kTitle ? CLUB_WEDGE : club_;
        float ang = shotAngle(club, aim_);
        float meter = charging_ ? meter_ : (mode_ == kTitle ? 0.55f + 0.12f * std::sin(t_ * 1.6f) : 0.62f);
        float spd = kClub[club].vmin + std::clamp(meter, 0.f, 1.f) * (kClub[club].vmax - kClub[club].vmin);
        Ball ghost = ball_;
        if (mode_ == kTitle) place(ghost, h.tee);
        launch(ghost, club, ang, spd);
        for (int i = 0; i < 14; i++) {
            for (int s = 0; s < 3; s++) {
                if (!ghost.rest && !ghost.holed && !ghost.wet) tick(ghost, club, kDt);
            }
            dotX[dots] = ghost.x - cam;
            dotY[dots] = kGround0 - ghost.y;
            dots++;
            if (ghost.rest || ghost.holed || ghost.wet) break;
        }
    }

    // Earlier sprites sit on top. Sky pictures go last so the ground covers them.
    if (mode_ == kTitle) spr(art_.logo, 160, 16, float(art_.logo.h), PAL_LOGO, false);
    if (mode_ == kWin) spr(art_.win, 160, 78, float(art_.win.h), PAL_LOGO, false);

    float ballH = ball_.holed ? 8.f : 11.f;
    float bx = ball_.x - cam;
    float by = kGround0 - ball_.y;
    spr(art_.ball, bx, by, ballH, PAL_BALL, false);

    bool faceRight = std::cos(aim_) >= 0.f;
    if (mode_ == kPlay && !ball_.rest && !kClub[flyClub_].putt) faceRight = ball_.vx >= 0.f;
    float manX = (ball_.rest || ball_.holed) ? ball_.x : lieX_;
    if (mode_ == kTitle) manX = h.tee;
    const gs::Mipped& man = art_.golfer[swingT_ > 0 ? 1 : 0];
    float mh = 34.f;
    spr(man, manX - cam + (faceRight ? -14.f : 14.f), worldY(manX) - mh * 0.5f, mh, PAL_MAN, !faceRight);
    for (int i = 0; i < dots; i++) spr(art_.dot, dotX[i], dotY[i], 4.f, PAL_AIM, false);

    const gs::Mipped& flag = art_.flag[(int(t_ * 6.f) & 1)];
    float pinX = h.cup + h.cupHalf * 0.35f - cam;
    float pinBase = kGround0 - h.lip;
    float fw = 30.f * float(flag.w) / float(std::max(1, int(flag.h)));
    {
        gs::Sprite s;
        s.h = 30;
        s.w = int16_t(std::max(1, int(std::lround(fw))));
        s.x = int16_t(std::lround(pinX - fw * (3.f / 18.f)));
        s.y = int16_t(std::lround(pinBase - 26.f));
        s.img = flag.pick(30.f);
        s.pal = PAL_FLAG;
        v.sprite(s);
    }
    spr(art_.shadow, bx, worldY(ball_.x) - 2.f, 6.f, PAL_SHADOW, false);

    const gs::Image& course = art_.course[hole_];
    gs::Sprite ground;
    ground.img = course;
    ground.x = int16_t(std::lround(-cam));
    ground.y = int16_t(kGround0 - kBaseRow);
    ground.w = int16_t(course.w);
    ground.h = int16_t(course.h);
    ground.pal = PAL_WORLD;
    v.sprite(ground);

    for (int i = 0; i < 3; i++) {
        float cx = std::fmod(30.f + i * 120.f + t_ * (8.f + i * 3.f), 380.f) - 30.f;
        spr(art_.cloud, cx, 22.f + (i == 1 ? 14.f : 0.f), 14.f, PAL_CLOUD, i == 2);
    }
    spr(art_.sun, 286, 20, 16, PAL_SUN, false);

    char buf[64];
    if (mode_ == kTitle) {
        hudC(4, "THREE HOLES", PAL_GOLD);
        hudC(5, "IN THE HOLE, NOT ON THE GREEN", PAL_WHITE);
        if (best_ > 0) {
            std::snprintf(buf, sizeof buf, "BEST %d", best_);
            hud(1, 4, buf, PAL_GREEN);
        }
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_GOLD);
        hudC(27, "ARROWS AIM   UP DOWN CLUB   HOLD Z", PAL_WHITE);
    } else if (mode_ == kWin) {
        hudC(12, "NOT ON THE GREEN", PAL_WHITE);
        std::snprintf(buf, sizeof buf, "THREE CUPS  STROKES %d", strokes_);
        hudC(14, buf, PAL_GOLD);
        if (best_ > 0) {
            std::snprintf(buf, sizeof buf, "BEST %d", best_);
            hudC(16, buf, PAL_GREEN);
        }
        hudC(26, "IN THE HOLE", PAL_GREEN);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
    } else if (mode_ == kLose) {
        hudC(12, "PICKED UP", PAL_RED);
        hudC(14, "THE BALL WAS NOT IN THE HOLE", PAL_WHITE);
        std::snprintf(buf, sizeof buf, "CUPS %d/3   STROKES %d", cups_, strokes_);
        hudC(16, buf, PAL_GOLD);
        hudC(27, "START RETRIES", PAL_GOLD);
    } else {
        std::snprintf(buf, sizeof buf, "HOLE %d  %s", hole_ + 1, h.name);
        hud(1, 0, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "PAR %d", h.par);
        hud(30, 0, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "STROKES %d", strokes_);
        hud(1, 1, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "CUPS %d/3", cups_);
        hud(28, 1, buf, PAL_GREEN);
        hudC(24, h.hint, PAL_GOLD);
        if (mode_ == kPause) {
            hudC(12, "PAUSED", PAL_GOLD);
            hudC(27, "START RESUME   ESC TITLE", PAL_WHITE);
        } else if (sayT_ > 0.f) {
            int pal = sayPal_;
            if (say_ == "ON THE GREEN" && (int(t_ * 4.f) & 1)) pal = PAL_GOLD;
            hudC(12, say_, pal);
        }
        if (ball_.rest && !ball_.holed) {
            int yards = int(std::lround(std::fabs(h.cup - ball_.x)));
            std::snprintf(buf, sizeof buf, "%s   %d TO THE CUP", lie_, yards);
            hudC(25, buf, ball_.kind == KIND_GREEN ? PAL_RED : PAL_WHITE);
        } else if (ball_.holed) {
            hudC(25, "IN THE HOLE", PAL_GREEN);
        } else {
            hudC(25, "IN FLIGHT", PAL_GOLD);
        }
        if (mode_ == kPlay || mode_ == kBanner) {
            if (charging_) {
                int n = int(std::lround(meter_ * 12.f));
                int pct = int(std::lround(meter_ * 100.f));
                float ang = shotAngle(club_, aim_);
                int loft = int(std::lround(std::atan2(std::sin(ang), std::fabs(std::cos(ang))) * (180.f / kPi)));
                std::string bar = kClub[club_].putt ? "PUTT  " : (std::string(kClub[club_].name) + " ");
                if (!kClub[club_].putt) {
                    std::snprintf(buf, sizeof buf, "%d ", loft);
                    bar += buf;
                }
                for (int i = 0; i < 12; i++) bar += (i < n) ? '=' : '.';
                std::snprintf(buf, sizeof buf, " %d", pct);
                bar += buf;
                hud(1, 26, bar, PAL_GOLD);
            } else if (ball_.rest && !ball_.holed) {
                float ang = shotAngle(club_, aim_);
                int loft = int(std::lround(std::atan2(std::sin(ang), std::fabs(std::cos(ang))) * (180.f / kPi)));
                if (kClub[club_].putt) {
                    std::snprintf(buf, sizeof buf, "PUTTER  %s", std::cos(ang) >= 0.f ? "RIGHT" : "LEFT");
                } else {
                    std::snprintf(buf, sizeof buf, "%s  LOFT %d", kClub[club_].name, loft);
                }
                hud(1, 26, buf, PAL_WHITE);
            }
            if (mode_ == kPlay) hud(1, 27, "ARROWS AIM  UP DOWN CLUB  HOLD Z", PAL_WHITE);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const float dt = 1.f / 60.f;
    t_ += dt;
    if (swingT_ > 0) swingT_--;
    if (sayT_ > 0.f) sayT_ = std::max(0.f, sayT_ - dt);
    const gs::Pad& pad = sys.pad;

    if (mode_ == kTitle) {
        const Hole& h = holeAt(0);
        if (!ball_.rest || ball_.x < 1.f) place(ball_, h.tee);
        ball_.rest = true;
        ball_.holed = false;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRound();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == kPause) {
        if (pad.pressed(gs::BTN_START)) mode_ = kPlay;
        else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = kTitle;
            hole_ = 0;
            loadHole();
            won_ = false;
            over_ = false;
        }
    } else if (mode_ == kBanner) {
        bannerT_ += dt;
        bool skip = !bot_ && pad.pressed(gs::BTN_START);
        if (skip || bannerT_ > (bot_ ? 0.2f : 0.75f)) {
            hole_++;
            if (hole_ >= kHoles) {
                mode_ = kWin;
                won_ = true;
                over_ = true;
            } else {
                loadHole();
                mode_ = kPlay;
            }
        }
    } else if (mode_ == kWin) {
        if (!bot_ && pad.pressed(gs::BTN_START)) startRound();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = kTitle;
            hole_ = 0;
            loadHole();
            won_ = false;
            over_ = false;
        }
    } else if (mode_ == kLose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) startRound();
    } else if (mode_ == kPlay) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE))) {
            mode_ = kPause;
        } else if (!ball_.rest && !ball_.holed && !ball_.wet && !ball_.ob) {
            bool skipped = ball_.skipped;
            for (int s = 0; s < kSub; s++) tick(ball_, flyClub_, kDt);
            shotFrames_++;
            if (shotFrames_ > kMaxShot && !ball_.holed && !ball_.wet && !ball_.ob) {
                ball_.vx = ball_.vy = 0;
                ball_.rolling = false;
                ball_.rest = true;
                if (ball_.inWell && ball_.y < holeAt(hole_).lip) {
                    ball_.holed = true;
                } else {
                    Surf s = groundAt(holeAt(hole_), ball_.x, false);
                    ball_.y = s.y + kR;
                    ball_.kind = s.kind;
                }
            }
            if (ball_.skipped || skipped) ball_.skipped = true;
            if (belowLip(ball_)) holeOut();
            else if (ball_.wet) replay("WET", PAL_RED);
            else if (ball_.ob) replay("OUT OF BOUNDS", PAL_RED);
            else if (ball_.rest) {
                lieX_ = ball_.x;
                lie_ = kindName(ball_.kind, false);
                if (ball_.skipped && ball_.kind == KIND_GREEN) say("TOO FAST", PAL_GOLD, 0.8f);
                if (holeStrokes_ >= kPickup) {
                    mode_ = kLose;
                    won_ = false;
                    over_ = true;
                    lie_ = "PICKED UP";
                }
            }
        } else if (belowLip(ball_)) {
            holeOut();
        } else if (ball_.rest) {
            if (needSettle_) settle();
            if (mode_ == kPlay && holeStrokes_ < kPickup) {
                if (bot_) {
                    Shot s = plan();
                    swing(s.club, s.ang, s.spd);
                } else {
                    const float rate = 1.15f;
                    if (std::fabs(pad.axisX) > 0.18f) aim_ += pad.axisX * rate * dt;
                    if (pad.down(gs::BTN_LEFT)) aim_ -= rate * dt;
                    if (pad.down(gs::BTN_RIGHT)) aim_ += rate * dt;
                    if (aim_ > kPi * 1.15f) aim_ = kPi * 1.15f;
                    if (aim_ < -0.4f) aim_ = -0.4f;
                    if (pad.pressed(gs::BTN_UP)) club_ = (club_ + CLUB_N - 1) % CLUB_N;
                    if (pad.pressed(gs::BTN_DOWN)) club_ = (club_ + 1) % CLUB_N;
                    bool hold = pad.down(gs::BTN_A) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
                    if (hold && !charging_) {
                        charging_ = true;
                        meter_ = 0;
                        meterDir_ = 1.f;
                    }
                    if (charging_ && hold) {
                        meter_ += meterDir_ * dt / 0.85f;
                        if (meter_ >= 1.f) {
                            meter_ = 1.f;
                            meterDir_ = -1.f;
                        }
                        if (meter_ <= 0.f) {
                            meter_ = 0.f;
                            meterDir_ = 1.f;
                        }
                    }
                    if (charging_ && !hold) {
                        float ang = shotAngle(club_, aim_);
                        float spd = kClub[club_].vmin + meter_ * (kClub[club_].vmax - kClub[club_].vmin);
                        swing(club_, ang, spd);
                    }
                }
            }
        }
    }

    chime(dt);
    draw();
}

}  // namespace golf

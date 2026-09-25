#include "game/shuffle.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <sstream>

namespace shuffle {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int SUB = 4;
constexpr float MU = 130.f;
constexpr float STOP = 3.f;
constexpr float R = 6.f;
constexpr float TABLE_L = 40.f;
constexpr float TABLE_R = 280.f;
constexpr float TABLE_FAR = 32.f;
constexpr float TABLE_NEAR = 168.f;
constexpr float Z3_TOP = 32.f, Z3_BOT = 56.f;
constexpr float Z2_TOP = 64.f, Z2_BOT = 88.f;
constexpr float Z1_TOP = 96.f, Z1_BOT = 120.f;
constexpr float LAUNCH_Y = 156.f;
constexpr float Y3 = 44.f;
constexpr float Y2 = 76.f;
constexpr float Y1 = 108.f;
constexpr int YOU = 0;
constexpr int HOUSE = 1;
constexpr float kYouX[4] = {100.f, 136.f, 172.f, 208.f};
constexpr float kHouseX[4] = {118.f, 154.f, 190.f, 226.f};

// A disk scores only after it has stopped fully inside one painted band.
// Touching a line, hanging off the wax, or dying in the gutter is nothing.
int scoreOf(const Disk& d) {
    if (!d.live || d.dead || !d.rest) return 0;
    float top = d.y - R, bot = d.y + R;
    float left = d.x - R, right = d.x + R;
    if (left < TABLE_L || right > TABLE_R || top < TABLE_FAR || bot > TABLE_NEAR) return 0;
    if (top >= Z3_TOP && bot <= Z3_BOT) return 3;
    if (top >= Z2_TOP && bot <= Z2_BOT) return 2;
    if (top >= Z1_TOP && bot <= Z1_BOT) return 1;
    return 0;
}

int tally(const std::vector<Disk>& ds, int side) {
    int n = 0;
    for (const auto& d : ds)
        if (d.side == side) n += scoreOf(d);
    return n;
}

void label(const Disk& d, std::string& say, int& pal) {
    if (!d.live || d.dead) {
        say = "OFF THE TABLE";
        pal = PAL_BAD;
        return;
    }
    int p = scoreOf(d);
    if (p == 3) {
        say = "IN THE 3";
        pal = PAL_GOLD;
        return;
    }
    if (p == 2) {
        say = "IN THE 2";
        pal = PAL_INK;
        return;
    }
    if (p == 1) {
        say = "IN THE 1";
        pal = PAL_INK;
        return;
    }
    float top = d.y - R, bot = d.y + R;
    auto over = [&](float a, float b) { return bot > a && top < b; };
    if (over(Z3_BOT, Z2_TOP) || over(Z2_BOT, Z1_TOP) || over(Z1_BOT, Z1_BOT + 8.f)) {
        say = "ON A LINE";
        pal = PAL_BAD;
        return;
    }
    if (bot > Z1_BOT) {
        say = "SHORT OF THE SCORE";
        pal = PAL_BAD;
        return;
    }
    say = "NOT IN THE SCORE";
    pal = PAL_BAD;
}

void freeStep(float& x, float& y, float& vx, float& vy, float h) {
    float sp = std::hypot(vx, vy);
    if (sp < STOP) {
        vx = vy = 0;
        return;
    }
    x += vx * h;
    y += vy * h;
    float drop = MU * h;
    if (sp <= drop) {
        vx = vy = 0;
        return;
    }
    float k = (sp - drop) / sp;
    vx *= k;
    vy *= k;
    if (std::hypot(vx, vy) < STOP) vx = vy = 0;
}

// Distance a straight shot travels. speedForRange inverts this same step.
float rangeOf(float speed) {
    float x = 0, y = 0, vx = 0, vy = -speed;
    const float h = DT / float(SUB);
    const int n = 60 * 8 * SUB;
    for (int i = 0; i < n; i++) {
        if (std::hypot(vx, vy) < STOP) break;
        freeStep(x, y, vx, vy, h);
    }
    return -y;
}

float speedForRange(float dist) {
    if (dist <= 1.f) return 0.f;
    float lo = 0.f, hi = 30.f;
    while (rangeOf(hi) < dist && hi < 5000.f) hi *= 2.f;
    for (int i = 0; i < 26; i++) {
        float mid = 0.5f * (lo + hi);
        if (rangeOf(mid) < dist) lo = mid;
        else hi = mid;
    }
    return hi;
}

bool killIfOff(Disk& d) {
    if (!d.live) return false;
    if (d.x >= TABLE_L && d.x <= TABLE_R && d.y >= TABLE_FAR && d.y <= TABLE_NEAR) return false;
    if (d.x < TABLE_L) d.x = TABLE_L - R * 0.55f;
    if (d.x > TABLE_R) d.x = TABLE_R + R * 0.55f;
    if (d.y < TABLE_FAR) d.y = TABLE_FAR - R * 0.55f;
    if (d.y > TABLE_NEAR) d.y = TABLE_NEAR + R * 0.55f;
    d.vx = d.vy = 0;
    d.live = false;
    d.dead = true;
    d.rest = true;
    return true;
}

bool collide(Disk& a, Disk& b) {
    if (!a.live || !b.live) return false;
    float dx = b.x - a.x, dy = b.y - a.y;
    float d2 = dx * dx + dy * dy;
    float minD = R * 2.f;
    if (d2 >= minD * minD || d2 < 1e-6f) return false;
    float dist = std::sqrt(d2);
    float nx = dx / dist, ny = dy / dist;
    float push = (minD - dist) * 0.5f;
    a.x -= nx * push;
    a.y -= ny * push;
    b.x += nx * push;
    b.y += ny * push;
    float rel = (b.vx - a.vx) * nx + (b.vy - a.vy) * ny;
    if (rel >= 0.f) return false;
    constexpr float E = 0.55f;
    float j = -(1.f + E) * rel * 0.5f;
    a.vx -= j * nx;
    a.vy -= j * ny;
    b.vx += j * nx;
    b.vy += j * ny;
    a.rest = b.rest = false;
    return true;
}

struct StepOut {
    int hits = 0;
    int gutters = 0;
};

StepOut stepDisks(std::vector<Disk>& disks) {
    StepOut out;
    const float h = DT / float(SUB);
    for (int s = 0; s < SUB; s++) {
        for (auto& d : disks) {
            if (!d.live) continue;
            if (d.vx == 0.f && d.vy == 0.f) {
                d.rest = true;
                continue;
            }
            freeStep(d.x, d.y, d.vx, d.vy, h);
            if (!std::isfinite(d.x) || !std::isfinite(d.y)) {
                d.live = false;
                d.dead = true;
                d.rest = true;
                d.vx = d.vy = 0;
                out.gutters++;
                continue;
            }
            if (killIfOff(d)) out.gutters++;
        }
        for (int pass = 0; pass < 3; pass++) {
            for (size_t i = 0; i < disks.size(); i++)
                for (size_t j = i + 1; j < disks.size(); j++)
                    if (collide(disks[i], disks[j])) out.hits++;
        }
        for (auto& d : disks) {
            if (!d.live) continue;
            if (killIfOff(d)) {
                out.gutters++;
                continue;
            }
            float sp = std::hypot(d.vx, d.vy);
            if (sp > 900.f) {
                d.vx *= 900.f / sp;
                d.vy *= 900.f / sp;
                sp = 900.f;
            }
            if (sp < STOP) {
                d.vx = d.vy = 0;
                d.rest = true;
            } else {
                d.rest = false;
            }
        }
    }
    return out;
}

void spawnInto(std::vector<Disk>& ds, int side, float x, float targetY) {
    float x0 = std::clamp(x, TABLE_L + R + 0.5f, TABLE_R - R - 0.5f);
    float dist = LAUNCH_Y - targetY;
    if (dist < 4.f) dist = 4.f;
    if (dist > 420.f) dist = 420.f;
    float spd = speedForRange(dist);
    Disk d;
    d.side = side;
    d.x = x0;
    d.y = LAUNCH_Y;
    d.vx = 0.f;
    d.vy = -spd;
    d.live = true;
    d.rest = false;
    d.dead = false;
    d.told = false;
    ds.push_back(d);
}

bool moving(const std::vector<Disk>& ds) {
    for (const auto& d : ds)
        if (d.live && !d.rest) return true;
    return false;
}

float distForMeter(float m) {
    float minD = 14.f;
    float maxD = LAUNCH_Y - (TABLE_FAR - 16.f);
    return minD + std::clamp(m, 0.f, 1.f) * (maxD - minD);
}

}  // namespace

void Game::beginFrame() {
    disks_.clear();
    thrown_[0] = thrown_[1] = 0;
    next_ = YOU;
    aimT_ = 0;
    rollT_ = 0;
    charging_ = false;
    ghostOn_ = false;
    sweet_ = false;
    meter_ = 0.2f;
    meterDir_ = 1.f;
    gainYou_ = gainHouse_ = 0;
    mode_ = Mode::Aim;
    say_ = "YOUR DISK";
    sayPal_ = PAL_INK;
}

void Game::layTable() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.A.clear();
    v.A.enabled = true;
    v.B.enabled = false;
    v.HUD.clear();
    v.A.scroll(0, 0);
    auto fill = [&](int c0, int c1, int r0, int r1, int tile, int pal, int hf = 0, int vf = 0) {
        for (int cy = r0; cy <= r1; cy++)
            for (int cx = c0; cx <= c1; cx++) v.A.set(cx, cy, gs::entry(tile, pal, hf, vf));
    };
    fill(2, 3, 3, 21, art_.wood, PAL_RAIL);
    fill(36, 37, 3, 21, art_.wood, PAL_RAIL);
    fill(3, 3, 3, 21, art_.vRail, PAL_RAIL, 0, 0);
    fill(36, 36, 3, 21, art_.vRail, PAL_RAIL, 1, 0);
    fill(2, 37, 2, 2, art_.hRail, PAL_RAIL, 0, 0);
    fill(2, 37, 22, 22, art_.hRail, PAL_RAIL, 0, 1);
    fill(4, 35, 3, 3, art_.gutter, PAL_GUTTER);
    fill(4, 35, 21, 21, art_.gutter, PAL_GUTTER);
    fill(4, 4, 4, 20, art_.gutter, PAL_GUTTER);
    fill(35, 35, 4, 20, art_.gutter, PAL_GUTTER);
    fill(5, 34, 4, 6, art_.wax, PAL_Z3);
    fill(5, 34, 7, 7, art_.line, PAL_LINE);
    fill(5, 34, 8, 10, art_.wax, PAL_Z2);
    fill(5, 34, 11, 11, art_.line, PAL_LINE);
    fill(5, 34, 12, 14, art_.wax, PAL_Z1);
    fill(5, 34, 15, 15, art_.line, PAL_LINE);
    fill(5, 34, 16, 20, art_.wax, PAL_WAX);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layTable();
    sys.vdp.setFogColor(gs::rgb4(2, 2, 2));
    sys.apu.setMaster(0.68f);
    sys.apu.setEcho(0.16f, 0.3f, 0.16f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        sys.vdp.lineFog[y] = 0;
        sys.vdp.road[y].on = false;
    }
    you_ = house_ = 0;
    frames_ = 0;
    over_ = won_ = false;
    aimX_ = 160.f;
    t_ = 0;
    disks_.clear();
    if (bot_) beginFrame();
    else mode_ = Mode::Title;
}

void Game::launch(int side, float x, float targetY) {
    spawnInto(disks_, side, x, targetY);
    mode_ = Mode::Roll;
    rollT_ = 0;
    charging_ = false;
    ghostOn_ = false;
    say_ = "SLIDING";
    sayPal_ = PAL_GOLD;
    if (!sys_) return;
    sys_->apu.noiseBurst(side == YOU ? 0.22f : 0.18f, side == YOU ? 1700.f : 980.f, 0.06f);
    sys_->rumble(0.1f, 0.32f, 34);
}

void Game::botLaunch() {
    int i = thrown_[YOU];
    if (i < 0 || i > 3) return;
    launch(YOU, kYouX[i], Y3);
    thrown_[YOU]++;
}

void Game::houseLaunch() {
    int i = thrown_[HOUSE];
    if (i < 0 || i > 3) return;
    float x = kHouseX[i];
    float y = bot_ ? Y1 : Y2;
    if (!bot_) {
        int baseH = tally(disks_, HOUSE);
        int baseY = tally(disks_, YOU);
        int best = -9999;
        for (int k = 0; k < 4; k++) {
            float tx = kHouseX[(i + k) % 4];
            Cast c = forecast(HOUSE, tx, Y2);
            int s = (c.house - baseH) * 5 - (c.you - baseY) * 4;
            if (s > best) {
                best = s;
                x = tx;
                y = Y2;
            }
        }
    }
    launch(HOUSE, x, y);
    thrown_[HOUSE]++;
}

Game::Cast Game::forecast(int side, float x, float targetY) const {
    auto ds = disks_;
    spawnInto(ds, side, x, targetY);
    for (int i = 0; i < 60 * 8; i++) {
        if (!moving(ds)) break;
        stepDisks(ds);
    }
    for (auto& d : ds) {
        d.vx = d.vy = 0;
        if (d.live) d.rest = true;
    }
    Cast c;
    c.disk = ds.back();
    c.you = tally(ds, YOU);
    c.house = tally(ds, HOUSE);
    return c;
}

void Game::updateGhost() {
    Cast c = forecast(YOU, aimX_, LAUNCH_Y - distForMeter(meter_));
    ghostOn_ = true;
    ghostX_ = c.disk.x;
    ghostY_ = c.disk.y;
    label(c.disk, say_, sayPal_);
}

void Game::humanAim(const gs::Pad& pad) {
    if (pad.down(gs::BTN_LEFT)) aimX_ -= 96.f * DT;
    if (pad.down(gs::BTN_RIGHT)) aimX_ += 96.f * DT;
    if (std::fabs(pad.axisX) > 0.12f) aimX_ += pad.axisX * 110.f * DT;
    aimX_ = std::clamp(aimX_, TABLE_L + R + 2.f, TABLE_R - R - 2.f);

    const bool held = pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO);
    const bool tapped = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C) ||
                        pad.pressed(gs::BTN_TURBO);
    if (pad.accel > 0.18f) {
        charging_ = true;
        meter_ = std::clamp((pad.accel - 0.18f) / 0.82f, 0.f, 1.f);
    } else if (tapped && !charging_) {
        charging_ = true;
        meter_ = 0.2f;
        meterDir_ = 1.f;
    }
    if (charging_ && (held || pad.accel > 0.18f)) {
        if (pad.accel <= 0.18f) {
            meter_ += meterDir_ * 0.38f * DT;
            if (meter_ >= 1.f) {
                meter_ = 1.f;
                meterDir_ = -1.f;
            } else if (meter_ <= 0.f) {
                meter_ = 0.f;
                meterDir_ = 1.f;
            }
        }
        updateGhost();
        bool good = sayPal_ == PAL_GOLD;
        if (good && !sweet_) sweetTick();
        sweet_ = good;
        return;
    }
    if (!charging_) {
        ghostOn_ = false;
        sweet_ = false;
        return;
    }
    if (thrown_[YOU] >= 4) {
        charging_ = false;
        return;
    }
    launch(YOU, aimX_, LAUNCH_Y - distForMeter(meter_));
    thrown_[YOU]++;
    sweet_ = false;
}

void Game::onRest() {
    rollT_ = 0;
    if (thrown_[YOU] >= 4 && thrown_[HOUSE] >= 4) {
        applyScore();
        return;
    }
    int other = 1 - next_;
    if (thrown_[other] < 4) next_ = other;
    mode_ = Mode::Aim;
    aimT_ = 0;
    charging_ = false;
    ghostOn_ = false;
    sweet_ = false;
    meter_ = 0.2f;
    meterDir_ = 1.f;
}

void Game::applyScore() {
    int gy = tally(disks_, YOU);
    int gh = tally(disks_, HOUSE);
    you_ += gy;
    house_ += gh;
    gainYou_ = gy;
    gainHouse_ = gh;
    frames_++;
    aimT_ = 0;
    if (you_ >= 15 && you_ > house_) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        chime(2);
    } else if (house_ >= 15 && house_ > you_) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        chime(0);
    } else {
        mode_ = Mode::Score;
        chime(gy > gh ? 1 : 0);
    }
}

void Game::noteStop(const Disk& d) {
    label(d, say_, sayPal_);
    if (!sys_) return;
    int p = scoreOf(d);
    if (d.dead) {
        sys_->apu.noiseBurst(0.2f, 160.f, 0.09f);
        sys_->rumble(0.28f, 0.12f, 70);
        return;
    }
    if (p == 3) sys_->apu.tone(0, 740.f, 0.06f);
    else if (p > 0) sys_->apu.tone(0, 494.f, 0.05f);
    else sys_->apu.tone(0, 174.f, 0.04f);
    toneT_ = 0.16f;
    sys_->rumble(p == 3 ? 0.22f : 0.06f, 0.16f, 36);
}

void Game::chime(int kind) {
    if (!sys_) return;
    if (kind >= 2) {
        sys_->apu.tone(0, 523.f, 0.1f);
        sys_->apu.tone(1, 659.f, 0.09f);
        sys_->apu.tone(2, 784.f, 0.08f);
        toneT_ = 0.7f;
        sys_->rumble(0.4f, 0.72f, 180);
    } else if (kind == 1) {
        sys_->apu.tone(0, 440.f, 0.07f);
        sys_->apu.tone(1, 659.f, 0.06f);
        toneT_ = 0.28f;
    } else {
        sys_->apu.tone(0, 196.f, 0.06f);
        toneT_ = 0.22f;
    }
}

void Game::quiet() {
    if (!sys_ || toneT_ <= 0.f) return;
    toneT_ -= DT;
    if (toneT_ <= 0.f) {
        sys_->apu.tone(0, 0, 0);
        sys_->apu.tone(1, 0, 0);
        sys_->apu.tone(2, 0, 0);
    }
}

void Game::sweetTick() {
    if (!sys_) return;
    sys_->apu.noiseBurst(0.07f, 2400.f, 0.035f);
    sys_->rumble(0.04f, 0.14f, 24);
    sweetT_ = 0.06f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || h < 1.f || m.h <= 0) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - w * 0.5f));
    s.y = int16_t(std::lround(cy - h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
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

void Game::puck(int side, float x, float y, float h) {
    spr(art_.puck, x, y, h, side == YOU ? PAL_YOU : PAL_HOUSE);
    spr(art_.shadow, x + 2.f, y + 3.f, h * 0.42f, PAL_YOU, false, true);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        if (y < 8) v.lineBackdrop[y] = gs::rgb4(1, 2, 3);
        else if (y < 20) v.lineBackdrop[y] = gs::rgb4(2, 3, 5);
        else if (y < 184) v.lineBackdrop[y] = gs::rgb4(3, 2, 2);
        else v.lineBackdrop[y] = gs::rgb4(1, 1, 2);
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    if (mode_ == Mode::Title) spr(art_.logo, 160, 8, float(art_.logo.h), PAL_LOGO);
    if (mode_ == Mode::Win) spr(art_.win, 160, 8, float(art_.win.h), PAL_LOGO);

    if (ghostOn_ && mode_ == Mode::Aim) spr(art_.puck, ghostX_, ghostY_, 12.f, PAL_AIM);
    if (mode_ == Mode::Aim && yourTurn() && mode_ != Mode::Title) {
        spr(art_.chev, aimX_, LAUNCH_Y + 8.f, 8.f, PAL_AIM);
        for (int i = 0; i < 3; i++) spr(art_.dot, aimX_, 146.f - float(i) * 8.f, 4.f, PAL_AIM);
    }

    const bool reveal = mode_ == Mode::Score || mode_ == Mode::Win || mode_ == Mode::Lose;
    if (reveal) {
        for (const auto& d : disks_) {
            int p = scoreOf(d);
            if (p > 0) spr(art_.digit[p - 1], d.x, d.y, 14.f, PAL_INK);
        }
    }

    const float markY[3] = {Y3, Y2, Y1};
    const int markD[3] = {2, 1, 0};
    for (int i = 0; i < 3; i++) {
        spr(art_.digit[markD[i]], 58.f, markY[i], 16.f, PAL_INK);
        spr(art_.digit[markD[i]], 262.f, markY[i], 16.f, PAL_INK);
    }

    if (mode_ == Mode::Title) {
        const float sx[6] = {136, 172, 154, 118, 190, 208};
        const float sy[6] = {Y3, Y3, Y2, Y1, Y1, Y2};
        const int side[6] = {YOU, YOU, YOU, HOUSE, HOUSE, HOUSE};
        for (int i = 0; i < 6; i++) puck(side[i], sx[i], sy[i], 13.f);
    } else {
        for (const auto& d : disks_) puck(d.side, d.x, d.y, d.dead ? 10.f : 13.f);
    }

    int youLeft = mode_ == Mode::Title ? 4 : 4 - thrown_[YOU];
    int houseLeft = mode_ == Mode::Title ? 4 : 4 - thrown_[HOUSE];
    if (youLeft < 0) youLeft = 0;
    if (houseLeft < 0) houseLeft = 0;
    for (int i = 0; i < youLeft; i++) puck(YOU, 8.f, 48.f + float(i) * 16.f, 11.f);
    for (int i = 0; i < houseLeft; i++) puck(HOUSE, 312.f, 48.f + float(i) * 16.f, 11.f);

    spr(art_.lamp, 28.f, 14.f, 24.f, PAL_LAMP);
    spr(art_.lamp, 292.f, 14.f, 24.f, PAL_LAMP, true);
    spr(art_.bottle, 8.f, 148.f, 26.f, PAL_LAMP);
    spr(art_.glass, 312.f, 156.f, 16.f, PAL_LAMP);

    const bool blink = (int(t_ * 2.2f) & 1) == 0;
    if (mode_ == Mode::Title) {
        hudC(23, "FIRST TO FIFTEEN", PAL_GOLD);
        hudC(24, "THE DISK HAS TO STOP IN THE SCORE", PAL_INK);
        hud(2, 25, "CREAM YOU", PAL_YOU);
        hud(16, 25, "1  2  3", PAL_GOLD);
        hud(28, 25, "RED HOUSE", PAL_HOUSE);
        hudC(26, "ARROWS AIM    HOLD Z", PAL_INK);
        if (blink) hudC(27, "PRESS START", PAL_GOLD);
        return;
    }

    char left[24], right[24], mid[24];
    std::snprintf(left, sizeof left, "YOU %d", you_);
    std::snprintf(right, sizeof right, "HOUSE %d", house_);
    std::snprintf(mid, sizeof mid, "TO 15");
    hud(1, 24, left, PAL_YOU);
    hudC(24, mid, PAL_GOLD);
    hud(40 - int(std::strlen(right)), 24, right, PAL_HOUSE);

    if (mode_ == Mode::Pause) {
        hudC(25, "PAUSED", PAL_GOLD);
        hudC(27, "START RESUME    ESC TITLE", PAL_INK);
        return;
    }
    if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        hudC(25, mode_ == Mode::Win ? "FIRST TO FIFTEEN" : "HOUSE REACHED FIFTEEN",
             mode_ == Mode::Win ? PAL_GOLD : PAL_BAD);
        char buf[48];
        std::snprintf(buf, sizeof buf, "YOU +%d   HOUSE +%d", gainYou_, gainHouse_);
        hudC(26, buf, gainYou_ > gainHouse_ ? PAL_YOU : PAL_HOUSE);
        if (!bot_) hudC(27, "START AGAIN", PAL_GOLD);
        return;
    }
    if (mode_ == Mode::Score) {
        char buf[48];
        std::snprintf(buf, sizeof buf, "YOU +%d   HOUSE +%d", gainYou_, gainHouse_);
        hudC(25, buf, gainYou_ > gainHouse_ ? PAL_GOLD : PAL_INK);
        hudC(26, "ONLY A DISK IN THE SCORE COUNTS", PAL_INK);
        if (!bot_) hudC(27, "START NEXT FRAME", PAL_GOLD);
        return;
    }

    std::string line;
    int linePal = PAL_INK;
    if (mode_ == Mode::Roll) {
        line = say_.empty() ? "SLIDING" : say_;
        linePal = sayPal_;
    } else if (charging_ && yourTurn()) {
        line = say_.empty() ? "SLIDING" : say_;
        linePal = sayPal_;
    } else if (yourTurn()) {
        line = "YOUR DISK " + std::to_string(thrown_[YOU] + 1) + " OF 4";
        linePal = PAL_YOU;
    } else {
        line = "HOUSE DISK " + std::to_string(std::min(thrown_[HOUSE] + 1, 4)) + " OF 4";
        linePal = PAL_HOUSE;
    }
    hudC(25, line, linePal);

    if (mode_ == Mode::Aim && yourTurn() && charging_) {
        int n = int(std::lround(meter_ * 16.f));
        if (n < 0) n = 0;
        if (n > 16) n = 16;
        std::string bar = "POWER ";
        for (int i = 0; i < 16; i++) bar += (i < n) ? '#' : '.';
        hud(8, 26, bar, sayPal_ == PAL_GOLD ? PAL_GOLD : PAL_INK);
    } else if (mode_ == Mode::Aim && yourTurn()) {
        hudC(26, "HOLD Z, LET GO IN THE SCORE", PAL_INK);
    } else if (mode_ == Mode::Roll) {
        hudC(26, "THE DISK HAS TO STOP", PAL_INK);
    } else {
        hudC(26, "HOUSE PLAYS THE 2", PAL_HOUSE);
    }
    hudC(27, "ARROWS AIM   HOLD Z   START PAUSE", PAL_INK);
}

std::string Game::dump() const {
    std::ostringstream o;
    o.setf(std::ios::fixed);
    o.precision(1);
    o << "mode " << int(mode_) << " next " << next_ << " thrown " << thrown_[0] << "," << thrown_[1];
    for (const auto& d : disks_) {
        o << (d.side == YOU ? " Y" : " H") << (d.live ? "" : "x") << d.x << "," << d.y << "=" << scoreOf(d);
    }
    return o.str();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (hitCd_ > 0.f) hitCd_ -= DT;
    if (sweetT_ > 0.f) sweetT_ -= DT;
    quiet();
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            you_ = house_ = 0;
            frames_ = 0;
            over_ = won_ = false;
            beginFrame();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
        else if (pad.pressed(gs::BTN_MODE)) {
            charging_ = false;
            ghostOn_ = false;
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            you_ = house_ = 0;
            frames_ = 0;
            over_ = won_ = false;
            beginFrame();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Score) {
        aimT_ += DT;
        if (aimT_ > (bot_ ? 0.08f : 1.4f) || (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)))) {
            beginFrame();
        }
    } else {
        if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            charging_ = false;
            ghostOn_ = false;
            mode_ = Mode::Title;
        } else if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = mode_;
            charging_ = false;
            ghostOn_ = false;
            mode_ = Mode::Pause;
        } else if (mode_ == Mode::Aim) {
            if (yourTurn()) {
                if (bot_) botLaunch();
                else humanAim(pad);
            } else {
                aimT_ += DT;
                if (bot_ || aimT_ >= 0.4f) houseLaunch();
            }
        }
        if (mode_ == Mode::Roll) {
            rollT_ += DT;
            StepOut step = stepDisks(disks_);
            if (step.hits && hitCd_ <= 0.f && sys_) {
                sys_->apu.noiseBurst(0.1f, 640.f, 0.04f);
                hitCd_ = 0.07f;
            }
            for (auto& d : disks_) {
                if (d.rest && !d.told) {
                    d.told = true;
                    noteStop(d);
                }
            }
            if (rollT_ > 7.f) {
                for (auto& d : disks_) {
                    d.vx = d.vy = 0;
                    d.rest = true;
                }
            }
            if (!moving(disks_) && !disks_.empty()) onRest();
        }
    }

    if (mode_ == Mode::Win) sys.setLight(210, 160, 40);
    else if (mode_ == Mode::Aim && yourTurn()) sys.setLight(180, 140, 40);
    else if (mode_ == Mode::Aim) sys.setLight(160, 30, 30);
    else if (mode_ == Mode::Title) sys.setLight(40, 30, 24);

    draw();
}

}  // namespace shuffle

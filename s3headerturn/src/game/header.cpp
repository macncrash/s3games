#include "header.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace headerturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kTau = 6.2831853f;
constexpr float kTip = 0.88f;
constexpr float kHeelK = 8.0f;
constexpr float kHeelDamp = 5.0f;
constexpr float kCap = 7.4f;
constexpr float kGlide = 1.4f;
constexpr float kYawBase = 0.32f;
constexpr float kYawPer = 0.048f;
constexpr float kClock = 110.f;
constexpr float kFocal = 260.f;
constexpr float kBoatH = 2.05f;
constexpr float kEnd = 396.f;

struct Bend {
    float z0, z1, x0, x1, exitZ;
    int dir;
};

constexpr Bend kBends[3] = {
    {52.f, 112.f, 0.f, 20.f, 132.f, 1},
    {162.f, 228.f, 20.f, -16.f, 250.f, -1},
    {280.f, 346.f, -16.f, 6.f, 366.f, 1},
};

float wrap(float a) {
    while (a > kPi) a -= kTau;
    while (a < -kPi) a += kTau;
    return a;
}

float smooth(float t) {
    t = std::clamp(t, 0.f, 1.f);
    return t * t * (3.f - 2.f * t);
}

float courseX(float z) {
    if (z <= kBends[0].z0) return kBends[0].x0;
    for (int i = 0; i < 3; i++) {
        const Bend& b = kBends[i];
        if (z < b.z1) {
            if (z < b.z0) return b.x0;
            float t = (z - b.z0) / (b.z1 - b.z0);
            return b.x0 + (b.x1 - b.x0) * smooth(t);
        }
    }
    return kBends[2].x1;
}

float courseHeading(float z) {
    float dx = courseX(z + 2.8f) - courseX(z - 2.8f);
    return std::atan2(dx, 5.6f);
}

float courseHalf(float z) {
    float h = 7.6f;
    for (const Bend& b : kBends) {
        if (z > b.z0 && z < b.z1) {
            float t = (z - b.z0) / (b.z1 - b.z0);
            h += 2.2f * std::sin(t * kPi);
        }
    }
    if (z < 22.f) h += (22.f - z) * 0.16f;
    if (z > kEnd - 18.f) h += 1.8f;
    return h;
}

bool inBend(float z) {
    for (const Bend& b : kBends)
        if (z >= b.z0 && z <= b.z1) return true;
    return false;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(int(ar + (br - ar) * t), int(ag + (bg - ag) * t), int(ab + (bb - ab) * t));
}

int hashN(int n) {
    uint32_t x = uint32_t(n) * 2246822519u;
    x ^= x >> 13;
    x *= 3266489917u;
    return int(x & 0x7fffffff);
}

}  // namespace

int Game::heelFrame() const {
    float u = std::clamp(heel_ / 0.78f, -1.f, 1.f);
    int f = int(std::lround((u + 1.f) * 0.5f * float(kHeels - 1)));
    return std::clamp(f, 0, kHeels - 1);
}

const char* Game::tipWhy() const {
    if (turns_ <= 0) return "tipped before the first turn";
    if (turns_ == 1) return "tipped on the second turn";
    if (turns_ == 2) return "tipped on the third turn";
    return "tipped before the end";
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (turns_ >= 2) return 3;
    if (turns_ >= 1 || inBend(z_)) return 2;
    return 1;
}

void Game::blip(float freq) { sys_->apu.tone(1, freq, 0.16f); }

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    why_ = "";
    turns_ = 0;
    for (bool& m : made_) m = false;
    t_ = 0;
    race_ = 0;
    x_ = 0;
    z_ = 16.f;
    heading_ = 0.08f;
    speed_ = 2.2f;
    sheet_ = 0.35f;
    yaw_ = 0;
    heel_ = 0.2f;
    heelVel_ = 0;
    shake_ = 0;
}

void Game::startRun() {
    mode_ = Mode::Run;
    over_ = false;
    won_ = false;
    why_ = "";
    turns_ = 0;
    for (bool& m : made_) m = false;
    race_ = 0;
    x_ = courseX(8.f);
    z_ = 8.f;
    heading_ = 0;
    speed_ = kGlide;
    sheet_ = 0.4f;
    yaw_ = 0;
    heel_ = 0;
    heelVel_ = 0;
    shake_ = 0;
    blip(220.f);
}

void Game::win() {
    mode_ = Mode::Win;
    over_ = true;
    won_ = true;
    why_ = "";
    blip(640.f);
    sys_->rumble(0.16f, 0.08f, 120);
}

void Game::fail(const char* why) {
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    why_ = why;
    shake_ = 1.f;
    blip(70.f);
    sys_->rumble(0.48f, 0.26f, 180);
}

void Game::buildCourse() {
    props_.clear();
    props_.reserve(80);
    props_.push_back(Prop{courseX(14.f) - courseHalf(14.f) - 7.f, 14.f, 3.6f, Kind::Shed, 0});
    for (int i = 0; i < 16; i++) {
        float z = 20.f + float(i) * 22.f;
        float c = courseX(z);
        float h = courseHalf(z);
        props_.push_back(Prop{c + h + 1.f, z, 1.6f, Kind::Stake, 0});
        props_.push_back(Prop{c - h - 1.f, z + 11.f, 1.6f, Kind::Stake, 0});
    }
    for (int i = 0; i < 14; i++) {
        float z = 28.f + float(i) * 24.f;
        float c = courseX(z);
        float h = courseHalf(z);
        int n = hashN(i * 17 + 3);
        float j = float(n % 100) / 100.f;
        float side = (i & 1) ? 1.f : -1.f;
        props_.push_back(Prop{c + side * (h + 2.6f + j * 1.6f), z, 1.2f + j * 0.4f, Kind::Reef, 0});
    }
    for (int i = 0; i < 3; i++) {
        const Bend& b = kBends[i];
        float z = 0.52f * (b.z0 + b.z1);
        float side = float(b.dir);
        props_.push_back(Prop{courseX(z) + side * (courseHalf(z) + 2.4f), z, 2.1f, Kind::Mark, i});
    }
    props_.push_back(Prop{courseX(kEnd) - courseHalf(kEnd) - 1.1f, kEnd, 2.8f, Kind::Finish, 0});
    props_.push_back(Prop{courseX(kEnd) + courseHalf(kEnd) + 1.1f, kEnd, 2.8f, Kind::Finish, 0});
    props_.push_back(Prop{18.f, 80.f, 0.7f, Kind::Gull, 0});
    props_.push_back(Prop{-22.f, 210.f, 0.7f, Kind::Gull, 1});
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildCourse();
    sys.apu.setMaster(0.7f);
    sys.apu.setEcho(0.12f, 0.22f, 0.1f);
    if (bot_) startRun();
    else showTitle();
}

void Game::controls(float& steer, float& sheet, bool& ease) {
    const gs::Pad& p = sys_->pad;
    steer = 0.f;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = std::clamp(p.axisX, -1.f, 1.f);
    const bool up = p.down(gs::BTN_UP) || p.down(gs::BTN_C);
    const bool down = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B);
    if (up) sheet_ = std::min(1.f, sheet_ + kDt * 0.85f);
    if (down) sheet_ = std::max(0.f, sheet_ - kDt * 1.2f);
    if (!up && !down) sheet_ += (0.32f - sheet_) * (1.f - std::exp(-0.55f * kDt));
    if (p.axisY > 0.25f) sheet_ = std::max(sheet_, p.axisY);
    if (p.accel > 0.08f) sheet_ = std::max(sheet_, p.accel);
    if (p.brake > 0.08f) sheet_ = std::min(sheet_, 1.f - p.brake);
    sheet = sheet_;
    ease = p.down(gs::BTN_A) || p.down(gs::BTN_Z) || p.down(gs::BTN_TURBO);
}

void Game::pilot(float& steer, float& sheet, bool& ease) {
    float look = std::clamp(16.f + speed_ * 0.35f, 16.f, 24.f);
    float ahead = courseX(z_ + look);
    float off = x_ - courseX(z_);
    float half = courseHalf(z_);
    float hDes = std::atan2(ahead - x_, look);
    hDes -= std::clamp(off * 0.08f, -0.32f, 0.32f);
    float err = wrap(hDes - heading_);
    float cmd = std::clamp(err / 0.32f, -1.f, 1.f);

    float curv = std::fabs(wrap(courseHeading(z_ + 22.f) - courseHeading(z_)));
    float want = 6.4f;
    if (curv > 0.1f) want = 5.4f;
    if (curv > 0.22f) want = 4.6f;
    if (std::fabs(off) > half * 0.4f) want = std::min(want, 3.4f);
    if (std::fabs(heel_) > 0.36f) want = std::min(want, 3.1f);

    float cap = 0.6f;
    if (speed_ > 6.0f) cap = 0.36f;
    if (std::fabs(heel_) > 0.42f) {
        bool adding = (cmd > 0.f) == (heel_ < 0.f);
        if (adding) cmd *= 0.25f;
        cap = std::min(cap, 0.34f);
    }
    steer = std::clamp(cmd, -cap, cap);
    sheet = std::clamp((want - speed_) * 0.5f + 0.28f, 0.08f, 0.82f);
    ease = std::fabs(heel_) > 0.16f || inBend(z_);
}

void Game::markTurns(float zPrev) {
    for (int i = 0; i < 3; i++) {
        if (made_[i]) continue;
        if (z_ < kBends[i].exitZ) break;
        if (i > 0 && !made_[i - 1]) {
            fail("skipped a turn");
            return;
        }
        if (zPrev >= kBends[i].exitZ) {
            fail("skipped a turn");
            return;
        }
        float off = std::fabs(x_ - courseX(z_));
        float herr = std::fabs(wrap(heading_ - courseHeading(z_)));
        if (off > courseHalf(z_) + 0.6f || herr > 0.95f || speed_ < 1.3f) {
            if (i == 0) fail("missed the first turn");
            else if (i == 1) fail("missed the second turn");
            else fail("missed the third turn");
            return;
        }
        made_[i] = true;
        turns_ = i + 1;
        blip(380.f + float(i) * 60.f);
        sys_->rumble(0.08f, 0.04f, 40);
    }
}

void Game::physics(float steer, float sheet, bool ease) {
    if (mode_ != Mode::Run) return;
    steer = std::clamp(steer, -1.f, 1.f);
    sheet = std::clamp(sheet, 0.f, 1.f);
    easing_ = ease;
    sheet_ = sheet;
    float zPrev = z_;

    float rate = kYawBase + std::min(speed_, kCap) * kYawPer;
    if (speed_ < 1.8f) rate *= 0.55f;
    if (ease) rate *= 0.78f;
    float yawCmd = steer * rate;
    yaw_ += (yawCmd - yaw_) * (1.f - std::exp(-6.f * kDt));
    heading_ = wrap(heading_ + yaw_ * kDt);

    float target = kGlide + sheet * (kCap - kGlide);
    speed_ += (target - speed_) * (1.f - std::exp(-1.5f * kDt));
    speed_ *= 1.f - std::fabs(steer) * speed_ * 0.0035f;
    speed_ = std::clamp(speed_, 0.35f, kCap);

    x_ += std::sin(heading_) * speed_ * kDt;
    z_ += std::cos(heading_) * speed_ * kDt;

    float load = -steer * speed_ * 0.06f - yaw_ * speed_ * 0.4f;
    if (ease) load *= 0.28f;
    float spring = kHeelK;
    float damp = kHeelDamp;
    if (ease) {
        spring += 6.f;
        damp += 4.f;
        load *= 0.5f;
    }
    heelVel_ += ((load - heel_) * spring - heelVel_ * damp) * kDt;
    heel_ += heelVel_ * kDt;

    float off = x_ - courseX(z_);
    float over = std::fabs(off) - courseHalf(z_);
    if (over > 0.f) {
        float sgn = off > 0.f ? 1.f : -1.f;
        x_ -= sgn * std::min(over, 0.28f);
        speed_ *= std::max(0.7f, 1.f - std::min(over, 2.f) * 0.08f);
        heel_ += sgn * std::min(over, 1.2f) * 0.045f;
        if (over > 3.2f) {
            fail("left the leg");
            return;
        }
    }
    heel_ = std::clamp(heel_, -1.3f, 1.3f);
    if (std::fabs(heel_) > kTip) {
        fail(tipWhy());
        return;
    }

    markTurns(zPrev);
    if (mode_ != Mode::Run) return;

    if (z_ >= kEnd) {
        bool lined = std::fabs(x_ - courseX(z_)) < courseHalf(z_) &&
                     std::fabs(wrap(heading_ - courseHeading(z_))) < 0.85f && speed_ > 1.2f;
        if (turns_ >= 3 && lined) win();
        else fail("missed the end");
        return;
    }
    race_ += kDt;
    if (race_ > kClock) {
        fail("missed the end");
        return;
    }

    wakeT_ -= kDt;
    if (wakeT_ <= 0.f && speed_ > 1.8f) {
        wakeT_ = 0.09f;
        Wake w;
        w.x = x_ - std::sin(heading_) * 1.5f;
        w.z = z_ - std::cos(heading_) * 1.5f;
        w.life = 1.f;
        wakes_[wakeN_] = w;
        wakeN_ = (wakeN_ + 1) % 10;
    }
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life -= kDt * 0.8f;
    bob_ = std::sin(t_ * (1.6f + sheet_ * 2.4f)) * 0.03f;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);

    slapT_ -= kDt;
    if (slapT_ <= 0.f && speed_ > 2.2f && mode_ == Mode::Run) {
        slapT_ = 0.5f - sheet_ * 0.12f;
        sys_->apu.noiseBurst(0.03f, 0.3f, 0.04f);
    }
}

void Game::audio() {
    if (!sys_) return;
    float hum = 0.f;
    if (mode_ == Mode::Run) hum = 0.03f + speed_ * 0.007f;
    else if (mode_ == Mode::Title) hum = 0.028f;
    float f = 80.f + speed_ * 7.f + sheet_ * 16.f;
    if (std::fabs(hum - tone0_) > 0.002f || mode_ == Mode::Run) sys_->apu.tone(0, f, hum);
    tone0_ = hum;
    if (mode_ != Mode::Run) sys_->apu.tone(1, 0.f, 0.f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        heel_ = 0.26f * std::sin(t_ * 0.7f);
        heading_ = 0.05f * std::sin(t_ * 0.4f);
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) startRun();
    } else if (mode_ == Mode::Run) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            float steer = 0, sheet = 0;
            bool ease = false;
            if (bot_) pilot(steer, sheet, ease);
            else controls(steer, sheet, ease);
            physics(steer, sheet, ease);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) showTitle();
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
        startRun();
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        showTitle();
    }
    audio();
    draw();
}

bool Game::project(float wx, float wy, float wz, float& sx, float& sy, float& scale, int& fog, float& rz) const {
    const float back = mode_ == Mode::Title ? 11.f : 7.8f;
    const float eye = mode_ == Mode::Title ? 3.5f : 2.6f;
    const float hor = mode_ == Mode::Title ? 74.f : 86.f;
    float camX = x_ - std::sin(heading_) * back;
    float camZ = z_ - std::cos(heading_) * back;
    float dx = wx - camX;
    float dz = wz - camZ;
    float S = std::sin(heading_);
    float C = std::cos(heading_);
    rz = dx * S + dz * C;
    float rx = dx * C - dz * S;
    if (rz < 0.9f) return false;
    scale = kFocal / rz;
    sx = 160.f + rx * scale;
    sy = hor - (wy - (eye + bob_)) * scale;
    fog = rz > 48.f ? std::clamp(int((rz - 48.f) / 16.f), 0, 12) : 0;
    return true;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -8 || cy + h * 0.5f < -8 || cx - w * 0.5f > gs::SCREEN_W + 8 || cy - h * 0.5f > gs::SCREEN_H + 8)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1600L);
    long sh = std::clamp(std::lround(h), 1L, 1600L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::drawSea() {
    gs::VDP& v = sys_->vdp;
    const float back = mode_ == Mode::Title ? 11.f : 7.8f;
    const float eye = mode_ == Mode::Title ? 3.5f : 2.6f;
    const float hor = mode_ == Mode::Title ? 74.f : 86.f;
    const float S = std::sin(heading_);
    const float C = std::cos(heading_);
    const float camX = x_ - S * back;
    const float camZ = z_ - C * back;
    const float camH = eye + bob_;
    const uint16_t zenith = gs::rgb4(3, 6, 12);
    const uint16_t mid = gs::rgb4(6, 10, 14);
    const uint16_t haze = gs::rgb4(11, 13, 14);
    const uint16_t deep = gs::rgb4(1, 3, 8);
    v.roadTime = int(t_ * 18.f);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (float(y) < hor) {
            float u = float(y) / std::max(hor, 1.f);
            v.lineBackdrop[y] = u < 0.55f ? lerpC(zenith, mid, u / 0.55f) : lerpC(mid, haze, (u - 0.55f) / 0.45f);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = std::max(1.f, float(y) - hor);
        float dist = camH * kFocal / row;
        float x0 = camX + S * dist;
        float z0 = camZ + C * dist;
        float c0 = courseX(z0);
        float c1 = (courseX(z0 + 1.4f) - courseX(z0 - 1.4f)) / 2.8f;
        float half = courseHalf(z0);
        float e0 = x0 - c0;
        float denom = C + c1 * S;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.pal = uint8_t(PAL_SEA);
        r.left = r.right = gs::GROUND_WATER;
        r.style = 2;
        r.v = z0 * 24.f;
        r.band = (int(std::floor(z0 * 0.3f)) & 1) ? 1 : 0;
        if (std::fabs(denom) < 0.04f) {
            r.cx = std::fabs(e0) <= half ? 160.f : -4000.f;
            r.hw = std::fabs(e0) <= half ? 900.f : 2.f;
        } else {
            float uA = (half - e0) / denom;
            float uB = (-half - e0) / denom;
            float scl = kFocal / std::max(dist, 0.35f);
            r.cx = 160.f + 0.5f * (uA + uB) * scl;
            r.hw = std::min(4000.f, 0.5f * std::fabs(uA - uB) * scl);
        }
        int fog = dist > 70.f ? std::clamp(int((dist - 70.f) / 26.f), 0, 11) : 0;
        v.lineFog[y] = uint8_t(fog);
        v.lineBackdrop[y] = deep;
    }
}

void Game::drawWorld() {
    struct Item {
        float rz, sx, sy, sh;
        const gs::Mipped* img;
        int pal, fog;
        bool shadow;
    };
    std::vector<Item> items;
    items.reserve(110);
    auto push = [&](float wx, float wy, float wz, const gs::Mipped& img, float worldH, int pal, bool shadow) {
        float sx, sy, scale;
        int fog;
        float rz;
        if (!project(wx, wy, wz, sx, sy, scale, fog, rz)) return;
        float sh = worldH * scale;
        if (sh < 1.2f || rz > 220.f) return;
        items.push_back(Item{rz, sx, sy, sh, &img, pal, fog, shadow});
    };

    for (const Prop& p : props_) {
        switch (p.kind) {
        case Kind::Stake: push(p.x, p.h * 0.5f, p.z, art_.stake, p.h, PAL_BUOY, false); break;
        case Kind::Reef: push(p.x, p.h * 0.5f, p.z, art_.reef, p.h, PAL_REEF, false); break;
        case Kind::Shed: push(p.x, p.h * 0.5f, p.z, art_.shed, p.h, PAL_SHED, false); break;
        case Kind::Finish: push(p.x, p.h * 0.5f, p.z, art_.finish, p.h, PAL_FINISH, false); break;
        case Kind::Mark: push(p.x, p.h * 0.5f, p.z, art_.mark[p.num], p.h, PAL_MARK, false); break;
        case Kind::Gull: {
            float bob = std::sin(t_ * 1.6f + p.z) * 0.12f;
            push(p.x, 3.2f + bob, p.z, art_.gull[int(t_ * 3.f + p.num) & 1], p.h, PAL_GULL, false);
            break;
        }
        }
    }
    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        push(w.x, 0.04f, w.z, art_.wake, 0.3f + (1.f - w.life) * 0.5f, PAL_FOAM, false);
    }

    float shake = shake_ * std::sin(t_ * 40.f) * 0.18f;
    bool tipped = mode_ == Mode::Fail && why_ && why_[0] == 't';
    push(x_ + shake, 0.04f, z_, art_.shade, 0.9f, PAL_BOAT, true);
    if (tipped) push(x_, 0.35f, z_, art_.wreck, 1.2f, PAL_BOAT, false);
    else push(x_ + shake, 0.9f, z_, art_.boat[heelFrame()], kBoatH, PAL_BOAT, false);

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.rz < b.rz; });
    for (const Item& it : items) spr(*it.img, it.sx, it.sy, it.sh, it.pal, it.fog, it.shadow);
}

void Game::drawSky() {
    float hor = mode_ == Mode::Title ? 74.f : 86.f;
    spr(art_.sun, 228.f, hor - 28.f, 14.f, PAL_SKY, 0, false);
    spr(art_.cloud, 48.f + std::sin(t_ * 0.06f) * 4.f, 18.f, 18.f, PAL_SKY, 0, false);
    spr(art_.cloud, 140.f + std::cos(t_ * 0.05f) * 6.f, 28.f, 12.f, PAL_SKY, 2, false);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::drawHud() {
    char buf[80];
    auto banner = [&](const gs::Mipped& m, float x, float y, int pal) { spr(m, x, y, float(m.h), pal, 0, false); };

    if (mode_ == Mode::Title) {
        banner(art_.title, 160.f, 22.f, PAL_BANNER);
        hudC(7, "MAKE THE THREE TURNS", PAL_HUD);
        hudC(8, "WITHOUT TIPPING", PAL_BANNER);
        hudC(9, "EASE THE SHEET IN THE BENDS", PAL_TAG);
        hudC(10, "MISSING THE END FAILS THE LEG", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        else hudC(26, "ARROWS STEER   UP SHEET   Z EASES", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        banner(art_.held, 160.f, 46.f, PAL_BANNER);
    } else if (mode_ == Mode::Fail) {
        bool tipped = why_ && why_[0] == 't';
        banner(tipped ? art_.tipped : art_.missed, 160.f, 38.f, PAL_ALERT);
    } else if (mode_ == Mode::Win) {
        banner(art_.made, 160.f, 34.f, PAL_WIN);
    }

    if (mode_ == Mode::Title) return;

    hud(1, 0, "S3 HEADER TURN", PAL_BANNER);
    if (mode_ == Mode::Win) std::snprintf(buf, sizeof buf, "MADE 3/3");
    else std::snprintf(buf, sizeof buf, "MADE %d/3", turns_);
    hud(31, 0, buf, PAL_TAG);

    if (mode_ == Mode::Pause) {
        hudC(16, "START CONTINUES", PAL_HUD);
        hudC(17, "MODE RETURNS", PAL_TAG);
        return;
    }
    if (mode_ == Mode::Win) {
        std::snprintf(buf, sizeof buf, "THE LEG  %.1fS", race_);
        hudC(8, buf, PAL_HUD);
        if (!bot_) hudC(26, "START SAILS IT AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(8, why_, PAL_ALERT);
        if (!bot_) hudC(26, "START TRIES AGAIN", PAL_HUD);
        return;
    }

    const char* line = "HOLD HER LEVEL";
    int pal = PAL_WIN;
    if (inBend(z_)) {
        std::snprintf(buf, sizeof buf, "TURN %d OF 3", std::min(turns_ + 1, 3));
        line = buf;
        pal = PAL_BANNER;
    } else if (turns_ == 3) {
        line = "FIND THE END";
        pal = PAL_TAG;
    } else if (turns_ > 0) {
        std::snprintf(buf, sizeof buf, "TURN %d MADE", turns_);
        line = buf;
        pal = PAL_WIN;
    }
    if (std::fabs(heel_) > kTip * 0.68f) {
        line = easing_ ? "EASED" : "EASE THE SHEET";
        pal = PAL_ALERT;
    }
    hud(1, 1, line, pal);
    std::snprintf(buf, sizeof buf, "SPD %.0f", speed_);
    hud(33, 1, buf, PAL_TAG);

    char bar[12];
    for (int i = 0; i < 11; i++) bar[i] = '-';
    bar[11] = 0;
    float n = std::clamp(heel_ / kTip, -1.f, 1.f);
    int at = std::clamp(int(std::lround((n + 1.f) * 5.f)), 0, 10);
    bar[at] = 'O';
    std::snprintf(buf, sizeof buf, "HEEL L%sR", bar);
    hud(1, 2, buf, std::fabs(n) > 0.7f ? PAL_ALERT : PAL_HUD);
    std::snprintf(buf, sizeof buf, "END %d", std::max(0, int(kEnd - z_)));
    hud(30, 2, buf, PAL_TAG);
    hud(1, 26, "ARROWS STEER   UP SHEET   Z EASES", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    drawSea();
    drawWorld();
    drawSky();
    drawHud();
}

}  // namespace headerturn

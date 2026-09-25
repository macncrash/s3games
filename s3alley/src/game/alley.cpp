#include "game/alley.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace alley {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float HORIZON = 80.f;
constexpr float FOCAL = 240.f;
constexpr float CAM_BACK = 7.0f;
constexpr float CAM_H = 3.70f;
constexpr float ALLEY = 2.20f;
constexpr float WALK = 7.1f;
constexpr float HURRY = 9.8f;
constexpr float DUCKS = 4.7f;
constexpr float CHASE = 5.4f;
constexpr float LAT = 3.6f;
constexpr float BODY = 0.10f;
constexpr float LANE = 0.60f;
constexpr float SIDE = 0.66f;
constexpr float WALK_X = 0.78f;
constexpr float DOOR_Z = 228.f;
constexpr float GAP0 = 8.5f;
constexpr float HIT_Z = 0.48f;
constexpr int LIVES = 3;

constexpr float WALL_W = 2.35f;
constexpr float WALL_H = 5.7f;
constexpr float WALL_X = ALLEY + 0.06f + WALL_W * 0.5f;

gs::FMOp op(float mul, float level, float ar, float dr, float sl, float rr) {
    gs::FMOp o{};
    o.mul = mul;
    o.level = level;
    o.ar = ar;
    o.dr = dr;
    o.sl = sl;
    o.rr = rr;
    return o;
}

uint16_t mix4(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    int r = int(std::lround(ch(a, 8) + (ch(b, 8) - ch(a, 8)) * t));
    int g = int(std::lround(ch(a, 4) + (ch(b, 4) - ch(a, 4)) * t));
    int bl = int(std::lround(ch(a, 0) + (ch(b, 0) - ch(a, 0)) * t));
    return gs::rgb4(r, g, bl);
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return mode_ == Mode::Pause ? 1 : 0;
    if (mode_ == Mode::Door) return 2;
    if (mode_ == Mode::Caught) return 3;
    return 1;
}

void Game::tune() {
    gs::FMPatch p{};
    p.alg = 4;
    p.vol = 0.10f;
    p.fb = 0.18f;
    p.echo = 0.42f;
    p.tone = 1600.f;
    p.op[0] = op(1.f, 0.35f, 0.03f, 0.4f, 0.9f, 0.3f);
    p.op[1] = op(1.f, 1.f, 0.04f, 0.45f, 1.f, 0.6f);
    p.op[2] = op(2.f, 0.16f, 0.03f, 0.35f, 0.7f, 0.4f);
    p.op[3] = op(1.f, 0.7f, 0.05f, 0.5f, 1.f, 0.6f);
    sys_->apu.setPatch(0, p);
    sys_->apu.setPatch(1, p);
    p.vol = 0.16f;
    p.echo = 0.22f;
    p.tone = 2400.f;
    sys_->apu.setPatch(2, p);
    sys_->apu.setEcho(0.32f, 0.48f, 0.20f);
    sys_->apu.keyOn(0, 110.f, 0.07f);
    sys_->apu.keyOn(1, 164.81f, 0.045f);
    sys_->apu.noise(0.016f, 2300.f, false);
    audio_ = true;
}

void Game::loadCourse() {
    struct Proto {
        float z, x, half;
        Kind kind;
        bool duck;
    };
    const Proto course[] = {
        {28, -SIDE, 0.27f, Kind::Crate, false},  {44, SIDE, 0.23f, Kind::Barrel, false},
        {60, 0, 0.27f, Kind::Crate, false},       {78, -SIDE, 0.27f, Kind::Crate, false},
        {78, SIDE, 0.23f, Kind::Barrel, false},   {96, 0, 0.24f, Kind::Sign, true},
        {112, SIDE, 0.27f, Kind::Crate, false},   {128, 0, 1.15f, Kind::Line, true},
        {146, -SIDE, 0.23f, Kind::Barrel, false}, {164, SIDE, 0.27f, Kind::Crate, false},
        {164, 0, 0.24f, Kind::Vent, false},       {182, -SIDE, 0.27f, Kind::Crate, false},
        {182, SIDE, 0.27f, Kind::Crate, false},   {200, 0.05f, 0.24f, Kind::Sign, true},
        {214, 0, 0.27f, Kind::Crate, false},
    };
    blocks_.clear();
    blocks_.reserve(sizeof course / sizeof course[0]);
    for (const auto& p : course) blocks_.push_back({p.z, p.x, p.half, p.kind, p.duck, false});
}

void Game::enterTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    hits_ = 0;
    z_ = 0;
    px_ = 0;
    camX_ = 0;
    gap_ = GAP0;
    t_ = 0;
    stun_ = shake_ = stepAcc_ = inv_ = 0;
    doorOpen_ = 0;
    fanStep_ = -1;
    loadCourse();
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    hits_ = 0;
    z_ = 0;
    px_ = 0;
    camX_ = 0;
    gap_ = GAP0;
    t_ = 0;
    stun_ = shake_ = stepAcc_ = inv_ = 0;
    doorOpen_ = 0;
    fanStep_ = -1;
    loadCourse();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    tune();
    if (bot_) begin();
    else enterTitle();
}

void Game::footstep() { sys_->apu.noiseBurst(0.09f, 420.f, 0.045f); }

void Game::hit(Block& b) {
    b.spent = true;
    hits_++;
    stun_ = 0.36f;
    shake_ = 0.32f;
    sys_->apu.noiseBurst(0.45f, 160.f, 0.18f);
    sys_->apu.keyOn(2, 90.f, 0.12f);
    sys_->rumble(0.55f, 0.25f, 140);
    if (hits_ >= LIVES) {
        mode_ = Mode::Caught;
        t_ = 0;
        won_ = false;
        sys_->apu.keyOff(0);
        sys_->apu.keyOff(1);
    }
}

void Game::bot(bool& left, bool& right, bool& duck) {
    left = right = duck = false;
    const float lane[3] = {-LANE, 0.f, LANE};
    const float spd = WALK;
    int best = 1;
    float bestScore = -1e9f;
    for (int i = 0; i < 3; i++) {
        float score = -std::fabs(lane[i]) * 0.35f - std::fabs(lane[i] - px_) * 0.12f;
        for (const auto& o : blocks_) {
            if (o.duck) continue;
            float dz = o.z - z_;
            if (dz < -0.7f || dz > 16.f) continue;
            float tLook = std::max(dz, 0.f) / spd + 0.28f;
            float reach = px_ + std::clamp(lane[i] - px_, -LAT * tLook, LAT * tLook);
            float rad = o.half + BODY + 0.05f;
            if (std::fabs(reach - o.x) < rad) {
                float u = 16.f - std::max(dz, 0.f);
                score -= u * u;
            }
        }
        if (score > bestScore) {
            bestScore = score;
            best = i;
        }
    }
    float want = lane[best];
    if (want < px_ - 0.03f) left = true;
    else if (want > px_ + 0.03f) right = true;

    for (const auto& o : blocks_) {
        if (!o.duck) continue;
        float dz = o.z - z_;
        if (dz < -0.35f || dz > 2.45f) continue;
        float tLook = std::max(dz, 0.f) / spd;
        float reach = px_ + std::clamp(want - px_, -LAT * tLook, LAT * tLook);
        if (o.kind == Kind::Line || std::fabs(reach - o.x) < o.half + BODY + 0.08f) duck = true;
    }
}

void Game::play(float dt) {
    bool left = false, right = false, duck = false, hurry = false;
    if (stun_ > 0) stun_ -= dt;
    if (inv_ > 0) inv_ -= dt;
    if (stun_ <= 0) {
        if (bot_) bot(left, right, duck);
        else {
            const gs::Pad& pad = sys_->pad;
            left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
            if (left && right) left = right = false;
            duck = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
            hurry = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_UP) || pad.down(gs::BTN_TURBO);
            if (duck) hurry = false;
        }
    }

    float lat = (right ? 1.f : 0.f) - (left ? 1.f : 0.f);
    px_ = std::clamp(px_ + lat * LAT * dt, -WALK_X, WALK_X);
    float speed = duck ? DUCKS : (hurry ? HURRY : WALK);
    if (stun_ > 0) speed = WALK * 0.35f;
    z_ += speed * dt;
    gap_ += (speed - CHASE) * dt;
    t_ += dt;
    bob_ = std::sin(z_ * 9.5f);
    if (shake_ > 0) shake_ -= dt;

    stepAcc_ += speed * dt;
    if (stepAcc_ > 0.62f && stun_ <= 0) {
        stepAcc_ = 0;
        footstep();
    }

    float dist = DOOR_Z - z_;
    float near = std::clamp(1.f - dist / 26.f, 0.f, 1.f);
    sys_->apu.setFreq(0, 110.f + near * 46.f);
    sys_->apu.setFreq(1, 164.81f + near * 30.f);
    sys_->setLight(int(30 + near * 180), int(24 + near * 70), 36);

    if (gap_ <= 0.15f) {
        mode_ = Mode::Caught;
        t_ = 0;
        won_ = false;
        sys_->apu.keyOff(0);
        sys_->apu.keyOff(1);
        sys_->apu.noiseBurst(0.5f, 120.f, 0.25f);
        return;
    }

    for (auto& o : blocks_) {
        if (o.spent) continue;
        if (std::fabs(z_ - o.z) > HIT_Z) continue;
        if (o.duck && duck) continue;
        if (inv_ > 0) continue;
        if (std::fabs(px_ - o.x) < o.half + BODY) {
            hit(o);
            inv_ = 0.85f;
            break;
        }
    }
    if (mode_ != Mode::Play) return;

    if (z_ >= DOOR_Z - 1.15f) {
        mode_ = Mode::Door;
        won_ = true;
        t_ = 0;
        doorOpen_ = 0;
        fanStep_ = 0;
        fanT_ = 0;
        z_ = DOOR_Z - 1.15f;
        sys_->apu.setFreq(0, 220.f);
        sys_->apu.setFreq(1, 329.63f);
        sys_->setLight(220, 140, 50);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C)) mode_ = Mode::Play;
        else if (sys.pad.pressed(gs::BTN_MODE)) enterTitle();
        draw();
        return;
    }
    if (mode_ == Mode::Title) {
        t_ += DT;
        bob_ = std::sin(t_ * 2.4f);
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_C)) begin();
        else if (sys.pad.pressed(gs::BTN_MODE)) sys.quit();
        draw();
        return;
    }
    if (mode_ == Mode::Door) {
        t_ += DT;
        doorOpen_ = std::min(1.f, doorOpen_ + DT * 0.75f);
        fanT_ += DT;
        if (fanStep_ >= 0 && fanStep_ < 4 && fanT_ > 0.18f * float(fanStep_)) {
            static const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
            sys.apu.keyOn(2, notes[fanStep_], 0.18f);
            fanStep_++;
        }
        if (doorOpen_ >= 1.f && t_ > 1.7f) over_ = true;
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_MODE))) enterTitle();
        draw();
        return;
    }
    if (mode_ == Mode::Caught) {
        t_ += DT;
        if (t_ > 1.6f) over_ = true;
        if (!bot_ && sys.pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && sys.pad.pressed(gs::BTN_MODE)) enterTitle();
        draw();
        return;
    }

    if (sys.pad.pressed(gs::BTN_MODE) && !bot_) {
        mode_ = Mode::Pause;
        draw();
        return;
    }
    play(DT);
    camX_ += (px_ - camX_) * 0.10f;
    draw();
}

int Game::fogOf(float depth) const {
    if (depth < 9.f) return 0;
    return std::min(16, int((depth - 9.f) * 0.42f));
}

void Game::screenSpr(const gs::Mipped& m, float cx, float cy, float h, float w, int pal, bool flip, bool shadow) {
    if (h < 1.1f || m.h < 1) return;
    if (w < 1.f) w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h));
    if (s.x > gs::SCREEN_W + 60 || s.x + s.w < -60 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -80) return;
    s.img = m.pick(float(s.h));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    s.fog = 0;
    sys_->vdp.sprite(s);
}

void Game::flushBills() {
    std::sort(bills_.begin(), bills_.end(), [](const Bill& a, const Bill& b) { return a.depth < b.depth; });
    for (const auto& b : bills_) {
        float h = b.h;
        float w = b.w;
        if (h < 1.1f) continue;
        gs::Sprite s;
        s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
        if (w < 1.f) w = h * float(b.img->w) / float(std::max(1, b.img->h));
        s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
        s.x = int16_t(std::lround(b.x - s.w * 0.5f));
        s.y = int16_t(std::lround(b.foot - s.h));
        if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 30 || s.y + s.h < -120) continue;
        s.img = b.img->pick(float(s.h));
        s.pal = uint8_t(b.pal);
        s.hflip = b.flip;
        s.shadow = b.shadow;
        s.fog = uint8_t(std::clamp(b.fog, 0, 16));
        sys_->vdp.sprite(s);
    }
    bills_.clear();
}

void Game::bill(const gs::Mipped& m, float wx, float wz, float worldH, float worldW, float lift, int pal, bool flip) {
    float depth = wz - (z_ - CAM_BACK);
    if (depth < 0.8f || depth > 80.f) return;
    float scale = FOCAL / depth;
    Bill b;
    b.depth = depth;
    b.x = 160.f + (wx - camX_) * scale;
    float sh = (shake_ > 0) ? std::sin(shake_ * 80.f) * shake_ * 10.f : 0.f;
    b.x += sh;
    b.foot = HORIZON + CAM_H * scale - lift * scale;
    b.h = worldH * scale;
    b.w = worldW > 0 ? worldW * scale : 0;
    b.img = &m;
    b.pal = pal;
    b.flip = flip;
    b.fog = fogOf(depth);
    b.shadow = false;
    bills_.push_back(b);
}

void Game::glyphText(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 13.f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        float gh = std::max(8.f, float(g.h) * scale);
        screenSpr(g, x + float(i) * adv + adv * 0.5f, y + gh, gh, 0, pal, false, false);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    const uint16_t skyTop = gs::rgb4(1, 1, 4);
    const uint16_t skyHor = gs::rgb4(8, 5, 6);
    const uint16_t brickC = gs::rgb4(3, 1, 2);
    const uint16_t brickFar = gs::rgb4(5, 2, 3);
    float sh = (shake_ > 0) ? std::sin(shake_ * 80.f) * shake_ * 8.f : 0.f;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        gs::RoadLine& r = v.road[y];
        r = gs::RoadLine{};
        if (y < int(HORIZON)) {
            float t = float(y) / HORIZON;
            v.lineBackdrop[y] = mix4(skyTop, skyHor, t);
            v.lineFog[y] = uint8_t((1.f - t) * 4.f);
            continue;
        }
        float sy = float(y) - HORIZON + 0.5f;
        float depth = CAM_H * FOCAL / sy;
        v.lineBackdrop[y] = mix4(brickFar, brickC, std::min(1.f, sy / 48.f));
        r.on = true;
        r.cx = 160.f - camX_ * (sy / CAM_H) + sh;
        r.hw = ALLEY * sy / CAM_H;
        float wv = z_ + depth;
        r.v = wv * 20.f;
        r.pal = PAL_FIELD;
        r.band = (int(std::floor(wv / 2.2f)) & 1) ? 1 : 0;
        r.style = 0;
        r.left = r.right = gs::GROUND_DROP;
        int fog = depth > 36.f ? 8 : int(depth / 36.f * 7.f);
        v.lineFog[y] = uint8_t(std::clamp(fog, 0, 8));
    }
}

void Game::queueWorld() {
    float z0 = std::floor((z_ - 3.f) / 1.45f) * 1.45f;
    for (float wz = z0; wz < z_ + 34.f; wz += 1.45f) {
        if (wz > DOOR_Z - 1.6f) break;
        int n = int(std::floor(wz / 1.45f));
        int vl = (n * 3) & 3;
        int vr = (n * 3 + 1) & 3;
        int pl = (n & 2) ? PAL_WARM : PAL_BRICK;
        int pr = (n & 2) ? PAL_BRICK : PAL_WARM;
        bill(art_.wall[vl], -WALL_X, wz, WALL_H, WALL_W, 0, pl, true);
        bill(art_.wall[vr], WALL_X, wz, WALL_H, WALL_W, 0, pr, false);
    }

    for (float wz = std::floor(z_ / 12.f) * 12.f; wz < z_ + 36.f && wz < DOOR_Z - 4.f; wz += 12.f) {
        if (wz < z_ - 2.f) continue;
        bill(art_.lamp, 0.f, wz, 0.85f, 0, 2.55f, PAL_LAMP, false);
        bill(art_.lamp, -WALL_X + 0.35f, wz + 6.f, 0.7f, 0, 2.1f, PAL_LAMP, false);
    }

    const float watchZ[3] = {70.f, 122.f, 176.f};
    const float watchX[3] = {-1.55f, 1.55f, -1.55f};
    for (int i = 0; i < 3; i++) bill(art_.walker[0], watchX[i], watchZ[i], 1.7f, 0, 0, PAL_SHADE, watchX[i] > 0);

    for (const auto& o : blocks_) {
        float lift = 0, h = 0.8f, w = 0;
        const gs::Mipped* img = &art_.crate;
        int pal = PAL_WOOD;
        switch (o.kind) {
            case Kind::Crate:
                img = &art_.crate;
                h = 0.95f;
                break;
            case Kind::Barrel:
                img = &art_.barrel;
                h = 1.05f;
                break;
            case Kind::Sign:
                img = &art_.sign;
                h = 1.15f;
                lift = 1.28f;
                break;
            case Kind::Line:
                img = &art_.laundry;
                h = 0.62f;
                w = 3.7f;
                lift = 1.18f;
                pal = PAL_CLOTH;
                break;
            case Kind::Vent:
                img = &art_.vent;
                h = 1.15f;
                pal = PAL_CLOTH;
                break;
        }
        bill(*img, o.x, o.z, h, w, lift, pal, false);
    }

    float slide = doorOpen_ * 0.55f;
    bill(art_.endwall, 0.f, DOOR_Z + 0.35f, 6.1f, 4.6f, 0, PAL_BRICK, false);
    if (doorOpen_ > 0.04f) bill(art_.glow, 0.f, DOOR_Z + 0.15f, 2.7f, 0.35f + doorOpen_ * 1.1f, 0, PAL_LAMP, false);
    bill(art_.door, -0.20f - slide, DOOR_Z, 2.55f, 0.52f, 0, PAL_WOOD, false);
    bill(art_.door, 0.20f + slide, DOOR_Z, 2.55f, 0.52f, 0, PAL_WOOD, true);

    bool crouch = false;
    if (mode_ == Mode::Play && stun_ <= 0) {
        if (bot_) {
            bool l, r, d;
            bot(l, r, d);
            crouch = d;
        } else
            crouch = sys_->pad.down(gs::BTN_DOWN) || sys_->pad.down(gs::BTN_B);
    }
    int frame = int(std::floor(z_ * 1.7f)) & 1;
    if (mode_ == Mode::Title) frame = int(std::floor(t_ * 2.f)) & 1;
    const gs::Mipped& body = crouch ? art_.crouch : art_.walker[frame];
    float bh = crouch ? 1.15f : 1.82f;
    float bob = (mode_ == Mode::Caught) ? 0 : std::fabs(bob_) * 0.06f;
    bill(art_.shadow, px_, z_ + 0.15f, 0.22f, 0.9f, -0.02f, PAL_COAT, false);
    if (bills_.empty() == false && bills_.back().img == &art_.shadow) bills_.back().shadow = true;
    bool flash = stun_ > 0 && (int(stun_ * 24.f) & 1);
    if (!flash) bill(body, px_, z_, bh, 0, bob, PAL_COAT, false);

    if (gap_ < 14.f && mode_ != Mode::Door) {
        float k = std::clamp(1.f - gap_ / 14.f, 0.f, 1.f);
        float depth = CAM_BACK + 0.4f + (1.f - k) * 3.5f;
        Bill b;
        b.depth = depth + 0.2f;
        b.x = 160.f + (-px_ * 0.4f - camX_) * (FOCAL / depth);
        b.foot = float(gs::SCREEN_H) + 8.f;
        b.h = 28.f + k * 86.f;
        b.w = 0;
        b.img = &art_.walker[frame];
        b.pal = PAL_SHADE;
        b.flip = px_ > 0;
        b.fog = int((1.f - k) * 8.f);
        b.shadow = false;
        bills_.push_back(b);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    sys_->vdp.HUD.clear();
    backdrop();

    char buf[32];
    int doorM = std::max(0, int(std::lround(DOOR_Z - z_)));
    std::snprintf(buf, sizeof buf, "DOOR %d", doorM);
    hud(1, 0, "S3 ALLEY", PAL_HUD);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_AMBER);

    std::string pips;
    for (int i = 0; i < LIVES; i++) {
        if (i) pips += ' ';
        pips += (i < hits_) ? '-' : 'O';
    }
    hud(1, 1, pips, hits_ ? PAL_ALERT : PAL_HUD);
    if (gap_ < 10.f && mode_ == Mode::Play) {
        std::snprintf(buf, sizeof buf, "BEHIND %.0f", std::max(0.f, gap_));
        hud(40 - int(std::strlen(buf)) - 1, 1, buf, PAL_ALERT);
    }

    if (mode_ == Mode::Title) {
        glyphText("S3 ALLEY", 160, 8, 2.f, PAL_AMBER, 0);
        glyphText("THE NARROW STREET", 160, 40, 1.f, PAL_HUD, 0);
        glyphText("THE FAR DOOR", 160, 56, 1.f, PAL_HUD, 0);
        hudC(26, "ENTER TO WALK", PAL_AMBER);
        hudC(27, "ARROWS  DOWN DUCK  C HURRY", PAL_HUD);
    } else if (mode_ == Mode::Play && t_ < 4.f) {
        hudC(26, "ARROWS  DOWN DUCK  C HURRY", PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        glyphText("PAUSED", 160, 96, 2.f, PAL_AMBER, 0);
        hudC(26, "ENTER WALKS  ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Door) {
        glyphText("THE FAR DOOR", 160, 16, 1.6f, PAL_AMBER, 0);
        glyphText("OPENS", 160, 42, 1.4f, PAL_HUD, 0);
    } else if (mode_ == Mode::Caught) {
        glyphText("THE STREET", 160, 18, 1.6f, PAL_ALERT, 0);
        glyphText("KEEPS YOU", 160, 44, 1.4f, PAL_HUD, 0);
        hudC(26, "ENTER RETRIES", PAL_HUD);
    }

    if (mode_ == Mode::Play) {
        for (const auto& o : blocks_) {
            float dz = o.z - z_;
            if (dz < 1.2f || dz > 7.5f || !o.duck) continue;
            if (o.kind == Kind::Line || std::fabs(px_ - o.x) < o.half + 0.2f) {
                hudC(24, "DUCK", PAL_AMBER);
                break;
            }
        }
    }

    bills_.clear();
    queueWorld();

    float doorDepth = (DOOR_Z - z_) + CAM_BACK;
    float doorH = 2.55f * FOCAL / std::max(1.f, doorDepth);
    if (doorH < 22.f && mode_ != Mode::Door) {
        float scale = FOCAL / std::max(8.f, doorDepth);
        float bx = 160.f + (0.f - camX_) * scale;
        screenSpr(art_.lamp, bx, HORIZON + 6.f, 12.f, 8.f, PAL_LAMP, false, false);
    }

    // Rain sits over the street. Text was queued as sprites already, so rain goes under it
    // only if submitted later — submit rain before the world, text already submitted.
    for (int i = 0; i < 26; i++) {
        float x = std::fmod(i * 53.f + float(sys_->frame) * 7.f, 340.f) - 10.f;
        float y = std::fmod(i * 91.f + float(sys_->frame) * 13.f, 250.f) - 8.f;
        screenSpr(art_.rain, x, y, 11.f, 4.f, PAL_RAIN, false, false);
    }
    flushBills();
}

}  // namespace alley

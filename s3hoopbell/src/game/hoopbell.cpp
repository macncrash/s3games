#include "game/hoopbell.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace hoopbell {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kSub = 8;
constexpr float kH = kDt / float(kSub);
constexpr float kG = 9.81f;
constexpr float kPi = 3.14159265f;
constexpr float kAngle = 51.f * kPi / 180.f;
constexpr float kRimX = 0.f;
constexpr float kRimY = 3.05f;
constexpr float kBallR = 0.12f;
constexpr float kTubeR = 0.02f;
constexpr float kRimR = 0.40f;
constexpr float kCountR = 0.14f;  // bell mouth; narrower than the rim
constexpr float kBoardX = 0.68f;
constexpr float kReleaseY = 2.05f;
constexpr float kHand = 0.30f;
constexpr float kBotX = -6.05f;
constexpr float kMinX = -7.20f;
constexpr float kMaxX = -4.50f;
constexpr float kMaxZ = 0.85f;
constexpr float kSweetLo = 0.42f;
constexpr float kSweetHi = 0.58f;
constexpr float kMeterRate = 0.80f;
constexpr float kFeetRate = 2.6f;
constexpr float kAimRate = 0.55f;
constexpr float kPx = 27.f;
constexpr float kRimScr = 248.f;
constexpr float kFloor = 201.f;
constexpr float kGlassCx = 36.f;
constexpr float kGlassCy = 50.f;
constexpr float kGlassScale = 50.f;
constexpr float kMeterX = 108.f;
constexpr float kMeterW = 124.f;
constexpr float kMeterY = 10.f;

float scrX(float x) { return kRimScr + x * kPx; }
float scrY(float y) { return kFloor - y * kPx; }
float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }
int rgbClamp(int v) { return std::max(0, std::min(15, v)); }

float speedScale(float m) {
    if (m >= kSweetLo && m <= kSweetHi) return 1.f;
    if (m > kSweetHi) return 1.12f + (m - kSweetHi) * 0.9f;
    return 0.88f - (kSweetLo - m) * 0.9f;
}

bool solve(float x0, float lat, float& vx, float& vy, float& vz) {
    float dx = kRimX - x0;
    float dz = lat;
    float dist = std::hypot(dx, dz);
    float cosA = std::cos(kAngle);
    float sinA = std::sin(kAngle);
    float rise = std::tan(kAngle) * dist - (kRimY - kReleaseY);
    if (rise < 0.25f || dist < 1.f) return false;
    float s2 = (0.5f * kG * dist * dist) / (cosA * cosA * rise);
    if (s2 <= 0.f) return false;
    float s = std::sqrt(s2);
    float vh = s * cosA;
    vx = vh * dx / dist;
    vz = vh * dz / dist;
    vy = s * sinA;
    return true;
}

bool launchBall(Ball& b, Trace& t, float feet, float lat, float meter) {
    float vx, vy, vz;
    if (!solve(feet + kHand, lat, vx, vy, vz)) return false;
    float sc = speedScale(meter);
    t = Trace{};
    b.x = feet + kHand;
    b.y = kReleaseY;
    b.z = 0.f;
    b.vx = vx * sc;
    b.vy = vy * sc;
    b.vz = vz * sc;
    return true;
}

void stepBall(Ball& b, Trace& t, float dt) {
    float px = b.x, py = b.y, pz = b.z;
    b.y += b.vy * dt - 0.5f * kG * dt * dt;
    b.vy -= kG * dt;
    b.x += b.vx * dt;
    b.z += b.vz * dt;
    float ix = b.x, iy = b.y, iz = b.z;

    if (b.y <= kBallR) {
        b.y = kBallR;
        if (b.vy < 0.f) b.vy = -b.vy * 0.42f;
        if (std::fabs(b.vy) < 0.8f) b.vy = 0.f;
        b.vx *= 0.90f;
        b.vz *= 0.90f;
    }

    if (t.clean) {
        b.vx += (kRimX - b.x) * 6.f * dt;
        b.vz += (0.f - b.z) * 6.f * dt;
        b.vx *= 0.82f;
        b.vz *= 0.82f;
        return;
    }

    float dx = b.x - kRimX;
    float dz = b.z;
    float d = std::hypot(dx, dz);
    float hole = kRimR - kBallR;
    if (d >= hole) {
        float ax = d < 1e-4f ? kRimR : kRimR * dx / d;
        float az = d < 1e-4f ? 0.f : kRimR * dz / d;
        float ox = b.x - (kRimX + ax);
        float oy = b.y - kRimY;
        float oz = b.z - az;
        float od = std::sqrt(ox * ox + oy * oy + oz * oz);
        float minD = kBallR + kTubeR;
        if (od < minD && od > 1e-5f) {
            float nx = ox / od, ny = oy / od, nz = oz / od;
            float pen = minD - od;
            b.x += nx * pen;
            b.y += ny * pen;
            b.z += nz * pen;
            float vn = b.vx * nx + b.vy * ny + b.vz * nz;
            if (vn < 0.f) {
                b.vx -= 1.55f * vn * nx;
                b.vy -= 1.55f * vn * ny;
                b.vz -= 1.55f * vn * nz;
            }
            t.rim = true;
        }
    }

    if (b.x + kBallR > kBoardX && b.vx > 0.f && std::fabs(b.z) < 0.85f && b.y > kRimY - 0.2f && b.y < kRimY + 1.2f) {
        b.x = kBoardX - kBallR;
        b.vx = -std::fabs(b.vx) * 0.55f;
        b.vy *= 0.92f;
        t.bank = true;
    }

    if (!t.crossed && py >= kRimY && iy < kRimY) {
        float den = py - iy;
        float u = den > 1e-8f ? (py - kRimY) / den : 1.f;
        float cx = px + (ix - px) * u;
        float cz = pz + (iz - pz) * u;
        t.crossed = true;
        t.crossX = cx;
        t.crossZ = cz;
        t.rad = std::hypot(cx - kRimX, cz);
        if (t.rad <= kCountR && !t.rim && !t.bank) {
            t.clean = true;
            t.scoreAt = t.flight;
        }
    }
}

bool settled(const Ball& b, const Trace& t) {
    if (t.clean && t.flight > t.scoreAt + 0.36f) return true;
    bool rest = b.y <= kBallR + 0.05f && std::fabs(b.vy) < 0.55f && std::hypot(b.vx, b.vz) < 0.65f && t.flight > 0.45f;
    bool gone = b.x < -10.f || b.x > 3.2f || std::fabs(b.z) > 4.f;
    return rest || gone || t.flight > 4.0f;
}

Kind classify(const Trace& t) {
    if (t.clean) return Kind::Swish;
    if (t.rim || t.bank) return Kind::Iron;
    if (!t.crossed || t.crossX < kRimX - 0.16f) return Kind::Short;
    if (t.crossX > kRimX + 0.18f) return Kind::Hot;
    return Kind::Miss;
}

struct Shot {
    Kind kind = Kind::Miss;
    float rad = 99.f;
    float crossX = 0;
    float crossZ = 0;
    bool rim = false;
    bool bank = false;
    bool crossed = false;
};

Shot cast(float feet, float lat, float meter) {
    Shot r;
    Ball b;
    Trace t;
    if (!launchBall(b, t, feet, lat, meter)) return r;
    const int cap = 60 * kSub * 5;
    for (int i = 0; i < cap; i++) {
        t.flight += kH;
        stepBall(b, t, kH);
        if (settled(b, t)) break;
    }
    r.kind = classify(t);
    r.rad = t.rad;
    r.crossX = t.crossX;
    r.crossZ = t.crossZ;
    r.rim = t.rim;
    r.bank = t.bank;
    r.crossed = t.crossed;
    return r;
}

const char* kindName(Kind k) {
    switch (k) {
    case Kind::Swish: return "SWISH";
    case Kind::Iron: return "IRON";
    case Kind::Short: return "SHORT";
    case Kind::Hot: return "HOT";
    case Kind::Miss: return "MISS";
    }
    return "MISS";
}

const char* hintWord(Kind k) {
    if (k == Kind::Swish) return "CLEAN";
    return kindName(k);
}

}  // namespace

const char* Game::phase() const {
    switch (mode_) {
    case Mode::Title: return "title";
    case Mode::Aim: return "aim";
    case Mode::Flight: return "flight";
    case Mode::Dead: return "dead";
    case Mode::Ring: return "ring";
    case Mode::Leave: return "leave";
    case Mode::Pause: return "pause";
    case Mode::Over: return "over";
    }
    return "?";
}

bool Game::audit() {
    struct T {
        float x, z, m;
        bool swish;
        const char* name;
    };
    const T tests[] = {
        {kBotX, 0.f, 0.50f, true, "TRUE"},   {kBotX, 0.f, 0.44f, true, "SWEETLO"},
        {kBotX, 0.f, 0.56f, true, "SWEETHI"}, {kBotX, 0.06f, 0.50f, true, "NEAR"},
        {kMinX, 0.f, 0.50f, true, "FAR"},     {kMaxX, 0.f, 0.50f, true, "CLOSE"},
        {kBotX, 0.f, 0.28f, false, "SHORT"},  {kBotX, 0.f, 0.78f, false, "HOT"},
        {kBotX, 0.50f, 0.50f, false, "WIDE"},
    };
    bool ok = true;
    for (const T& t : tests) {
        Shot r = cast(t.x, t.z, t.m);
        bool sw = r.kind == Kind::Swish;
        if (sw != t.swish) {
            std::fprintf(stderr, "s3hoopbell %s -> %s rad %.3f cross %.3f %.3f rim %d bank %d\n", t.name, kindName(r.kind),
                         r.rad, r.crossX, r.crossZ, r.rim ? 1 : 0, r.bank ? 1 : 0);
            ok = false;
        }
    }
    return ok;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    rules_ = audit();
    if (!rules_) std::fprintf(stderr, "s3hoopbell rules failed\n");
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 3));
    sys.apu.setMaster(0.70f);
    toTitle();
}

void Game::toTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rung_ = false;
    dead_ = 0;
    tryNo_ = 0;
    why_ = "";
    last_ = Kind::Miss;
    hint_ = Kind::Miss;
    bellAmp_ = 0.18f;
    bellPh_ = 0.f;
    feet_ = kBotX;
    aimZ_ = 0.f;
    meter_ = 0.f;
    meterDir_ = 1.f;
    clock_ = 0.f;
    flash_ = 0.f;
    wasSweet_ = false;
    ball_ = Ball{};
}

void Game::newGame() {
    dead_ = 0;
    tryNo_ = 0;
    won_ = false;
    over_ = false;
    rung_ = false;
    why_ = "";
    last_ = Kind::Miss;
    bellAmp_ = 0.18f;
    feet_ = kBotX;
    aimZ_ = 0.f;
    flash_ = 0.f;
    beginAim();
}

void Game::beginAim() {
    meter_ = 0.f;
    meterDir_ = 1.f;
    wasSweet_ = false;
    deadT_ = 0.f;
    hint_ = Kind::Miss;
    rimSnd_ = false;
    bankSnd_ = false;
    tr_ = Trace{};
    mode_ = Mode::Aim;
    if (bot_) {
        feet_ = kBotX;
        aimZ_ = 0.f;
    }
    feet_ = clampf(feet_, kMinX, kMaxX);
    aimZ_ = clampf(aimZ_, -kMaxZ, kMaxZ);
}

bool Game::launch(float meter) {
    if (mode_ != Mode::Aim) return false;
    if (!launchBall(ball_, tr_, feet_, aimZ_, meter)) return false;
    rimSnd_ = false;
    bankSnd_ = false;
    mode_ = Mode::Flight;
    if (sys_) {
        sys_->apu.noiseBurst(0.14f, 1900.f, 0.04f);
        blip(0, 520.f, 0.05f, 0.06f);
    }
    return true;
}

void Game::fly(float dt) {
    (void)dt;
    for (int i = 0; i < kSub; i++) {
        tr_.flight += kH;
        stepBall(ball_, tr_, kH);
        if (tr_.rim && !rimSnd_) {
            rimSnd_ = true;
            sys_->apu.noiseBurst(0.34f, 2100.f, 0.07f);
            blip(2, 180.f, 0.05f, 0.06f);
        }
        if (tr_.bank && !bankSnd_) {
            bankSnd_ = true;
            sys_->apu.noiseBurst(0.28f, 640.f, 0.08f);
        }
        if (settled(ball_, tr_)) {
            finishShot();
            return;
        }
    }
}

void Game::finishShot() {
    if (mode_ != Mode::Flight) return;
    last_ = classify(tr_);
    if (last_ == Kind::Swish && rules_) {
        ring();
        return;
    }
    why_ = kindName(last_);
    dieTry();
}

void Game::ring() {
    if (rung_ || mode_ != Mode::Flight) return;
    rung_ = true;
    won_ = true;
    tryNo_ = dead_ + 1;
    why_ = "BELL";
    bellAmp_ = 1.f;
    bellTick_ = 0.02f;
    ringT_ = 0.f;
    flash_ = 0.45f;
    mode_ = Mode::Ring;
    if (!sys_) return;
    sys_->apu.noise(0, 1000);
    sys_->rumble(0.4f, 0.8f, 180);
    sys_->setLight(255, 196, 48);
}

void Game::dieTry() {
    if (rung_ || mode_ != Mode::Flight) return;
    dead_++;
    if (sys_) sys_->apu.noise(0, 1000);
    if (dead_ >= 3) {
        mode_ = Mode::Over;
        won_ = false;
        over_ = true;
        flash_ = 0.2f;
        if (sys_) {
            blip(0, 90.f, 0.1f, 0.4f);
            sys_->apu.tone(1, 64.f, 0.06f);
            tickT_ = 0.4f;
            sys_->setLight(150, 28, 28);
        }
        return;
    }
    mode_ = Mode::Dead;
    deadT_ = 0.f;
    if (sys_) {
        float f = last_ == Kind::Hot ? 110.f : 150.f;
        blip(0, f, 0.08f, 0.22f);
        sys_->setLight(120, 48, 28);
    }
}

void Game::botAim(float dt) {
    feet_ = kBotX;
    aimZ_ = 0.f;
    float prev = meter_;
    meter_ += meterDir_ * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    bool up = meter_ >= prev;
    if ((up && prev < 0.50f && meter_ >= 0.50f) || (!up && prev > 0.50f && meter_ <= 0.50f)) launch(0.50f);
}

void Game::humanAim(const gs::Pad& p, bool fire, float dt) {
    if (fire) {
        launch(meter_);
        return;
    }
    float mx = 0.f, my = 0.f;
    if (p.down(gs::BTN_LEFT)) mx -= 1.f;
    if (p.down(gs::BTN_RIGHT)) mx += 1.f;
    if (p.down(gs::BTN_UP)) my -= 1.f;
    if (p.down(gs::BTN_DOWN)) my += 1.f;
    if (std::fabs(p.axisX) > 0.25f) mx = p.axisX;
    if (std::fabs(p.axisY) > 0.25f) my = -p.axisY;
    feet_ = clampf(feet_ + mx * kFeetRate * dt, kMinX, kMaxX);
    aimZ_ = clampf(aimZ_ + my * kAimRate * dt, -kMaxZ, kMaxZ);
    meter_ += meterDir_ * kMeterRate * dt;
    if (meter_ >= 1.f) {
        meter_ = 1.f;
        meterDir_ = -1.f;
    } else if (meter_ <= 0.f) {
        meter_ = 0.f;
        meterDir_ = 1.f;
    }
    hint_ = cast(feet_, aimZ_, meter_).kind;
    bool sweet = hint_ == Kind::Swish;
    if (sweet && !wasSweet_) blip(2, 988.f, 0.04f, 0.045f);
    wasSweet_ = sweet;
}

void Game::dribble() {
    float s = std::sin(clock_ * 7.4f);
    ball_.x = feet_ + 0.42f;
    ball_.y = 0.18f + std::fabs(s) * 0.85f;
    ball_.z = 0.f;
    ball_.vx = ball_.vy = ball_.vz = 0.f;
    if (s > 0.f && dribS_ <= 0.f && mode_ != Mode::Pause && sys_) sys_->apu.noiseBurst(0.08f, 240.f, 0.03f);
    dribS_ = s;
}

void Game::blip(int ch, float freq, float vol, float hold) {
    if (!sys_) return;
    sys_->apu.tone(ch, freq, vol);
    if (ch == 2) tickT_ = hold;
    else toneT_ = hold;
}

void Game::pump(float dt) {
    if (!sys_) return;
    bool chiming = rung_ && (mode_ == Mode::Ring || mode_ == Mode::Leave || (mode_ == Mode::Over && won_));
    if (chiming) {
        bellTick_ -= dt;
        if (bellTick_ <= 0.f && bellAmp_ > 0.35f) {
            float vol = 0.05f + 0.08f * bellAmp_;
            sys_->apu.tone(0, 740.f, vol);
            sys_->apu.tone(1, 1110.f, vol * 0.7f);
            toneT_ = 0.16f;
            bellTick_ = 0.26f;
        }
    }
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f) {
            sys_->apu.tone(0, 0, 0);
            if (!chiming) sys_->apu.tone(1, 0, 0);
        }
    }
    if (tickT_ > 0.f) {
        tickT_ -= dt;
        if (tickT_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (mode_ == Mode::Flight) sys_->apu.noise(0.025f, 1600.f, true);
    else sys_->apu.noise(0, 1000);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += kDt;
    if (flash_ > 0.f) flash_ = std::max(0.f, flash_ - kDt);
    bellPh_ += kDt * (rung_ ? 14.f : 2.2f);
    if (rung_) bellAmp_ = std::max(0.16f, bellAmp_ - kDt * 0.42f);

    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START);
    bool back = p.pressed(gs::BTN_MODE);
    bool fire = p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_C) || p.pressed(gs::BTN_TURBO);
    if (bot_) {
        start = false;
        back = false;
        fire = false;
        if (mode_ == Mode::Title && clock_ > 0.35f) start = true;
    }

    Mode before = mode_;
    if (mode_ == Mode::Pause) {
        if (start || fire) mode_ = held_;
        else if (back) toTitle();
    } else if (mode_ == Mode::Title) {
        if (start || fire) newGame();
        else if (back) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Over) {
        if (!bot_ && (start || fire)) newGame();
        else if (!bot_ && back) toTitle();
    } else if (!bot_ && (mode_ == Mode::Aim || mode_ == Mode::Flight) && (start || back)) {
        held_ = mode_;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Aim) {
        if (bot_) botAim(kDt);
        else humanAim(p, fire, kDt);
    } else if (mode_ == Mode::Flight && before == Mode::Flight) {
        fly(kDt);
    } else if (mode_ == Mode::Dead && before == Mode::Dead) {
        deadT_ += kDt;
        if (deadT_ > 0.72f || (!bot_ && (fire || start))) beginAim();
    } else if (mode_ == Mode::Ring && before == Mode::Ring) {
        ringT_ += kDt;
        if (ringT_ > 0.72f) {
            mode_ = Mode::Leave;
            leaveT_ = 0.f;
        }
    } else if (mode_ == Mode::Leave && before == Mode::Leave) {
        leaveT_ += kDt;
        feet_ -= 3.6f * kDt;
        if (leaveT_ > 1.25f) {
            mode_ = Mode::Over;
            won_ = true;
            over_ = true;
        }
    }

    if (mode_ == Mode::Title || mode_ == Mode::Aim || (mode_ == Mode::Pause && (held_ == Mode::Aim || held_ == Mode::Title)))
        dribble();

    pump(kDt);
    draw();
}

void Game::spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip, bool shadow) {
    if (!sys_ || w < 1.f || h < 1.f || img.w == 0) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = img;
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h <= 0) return;
    float w = h * float(m.w) / float(m.h);
    spr(m.pick(h), cx, cy, w, h, pal, flip, false);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!sys_ || !s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = 0;
    if (s)
        while (s[n]) n++;
    hud(20 - n / 2, row, s, pal);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineFog[y] = 0;
        v.road[y].on = false;
        int boost = flash_ > 0.f && y < 90 ? 2 : 0;
        if (y < 168) {
            float u = y / 168.f;
            int r = rgbClamp(int(3 + u * 9) + boost);
            int g = rgbClamp(int(3 + u * 3) + (flash_ > 0.f ? 1 : 0));
            int b = rgbClamp(int(8 - u * 5));
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else if (y < int(kFloor)) {
            v.lineBackdrop[y] = gs::rgb4(4, 3, 3);
        } else {
            int w = 5 + ((y / 3) & 1);
            v.lineBackdrop[y] = gs::rgb4(w, w - 1, 2);
        }
    }
}

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();

    auto word = [&](const gs::Image& img, int pal, float y) { spr(img, 168.f, y, float(img.w), float(img.h), pal); };

    if (mode_ == Mode::Title) {
        word(art_.hoop, PAL_GOLD, 18.f);
        word(art_.bellWord, PAL_GOLD, 42.f);
    } else if (mode_ == Mode::Ring) {
        word(art_.rung, PAL_GREEN, 22.f);
    } else if (mode_ == Mode::Leave || (mode_ == Mode::Over && won_)) {
        word(art_.leaveW, PAL_GOLD, 22.f);
    } else if (mode_ == Mode::Over) {
        word(art_.deadW, PAL_ALERT, 22.f);
    } else if (mode_ == Mode::Dead) {
        const gs::Image* img = &art_.missW;
        if (last_ == Kind::Short) img = &art_.shortW;
        else if (last_ == Kind::Hot) img = &art_.hotW;
        else if (last_ == Kind::Iron) img = &art_.ironW;
        word(*img, PAL_ALERT, 22.f);
    }

    float swing = std::sin(bellPh_) * (rung_ ? 11.f * bellAmp_ : 1.7f);
    float rimSy = scrY(kRimY);
    float bellH = 34.f;
    float bellCy = rimSy + bellH * 0.46f;
    float bellCx = scrX(kRimX) + swing;

    if (rung_ && bellAmp_ > 0.4f) {
        for (int i = 0; i < 6; i++) {
            float a = bellPh_ * 1.4f + float(i) * 1.047f;
            float rad = 14.f + (1.f - bellAmp_) * 16.f;
            spr(art_.pip, bellCx + std::cos(a) * rad, bellCy + std::sin(a) * rad * 0.45f, 4.f, 4.f, PAL_GOLD);
        }
    }

    int spin = (int(tr_.flight * 14.f) + int(clock_ * 8.f)) & 1;
    float ballH = 16.f;
    sprM(art_.ball[spin], scrX(ball_.x), scrY(ball_.y) + ball_.z * 10.f, ballH, PAL_BALL);

    spr(art_.clapper, bellCx + swing * 0.25f, bellCy + 6.f, 7.f, 12.f, PAL_BELL);
    sprM(art_.bell, bellCx, bellCy, bellH, PAL_BELL);

    int netFrame = (rung_ ? int(bellPh_ * 2.f) : int(clock_ * 3.f)) & 1;
    spr(art_.net[netFrame], scrX(kRimX) - 1.f, rimSy + 18.f, 32.f, 30.f, PAL_IRON);
    spr(art_.rim, scrX(kRimX) - 2.f, rimSy, 44.f, 15.f, PAL_IRON);

    bool showAim = mode_ == Mode::Aim || mode_ == Mode::Title || (mode_ == Mode::Pause && held_ != Mode::Flight);
    if (showAim || mode_ == Mode::Flight || mode_ == Mode::Ring) {
        if (mode_ == Mode::Flight || mode_ == Mode::Ring) {
            float along = ball_.x - kRimX;
            if (std::fabs(along) < 1.15f && std::fabs(ball_.z) < 0.9f) {
                float px = clampf(kGlassCx + ball_.z * kGlassScale, kGlassCx - 26.f, kGlassCx + 26.f);
                float py = clampf(kGlassCy - along * kGlassScale, kGlassCy - 26.f, kGlassCy + 26.f);
                spr(art_.pip, px, py, 7.f, 7.f, PAL_BALL);
            }
        }
        if (showAim) {
            float bob = mode_ == Mode::Title ? std::sin(clock_ * 3.f) * 1.1f : 0.f;
            float ax = clampf(kGlassCx + aimZ_ * kGlassScale, kGlassCx - 26.f, kGlassCx + 26.f);
            int pal = PAL_GOLD;
            if (mode_ == Mode::Aim) {
                if (hint_ == Kind::Swish) pal = PAL_GREEN;
                else if (hint_ == Kind::Short || hint_ == Kind::Hot) pal = PAL_ALERT;
            }
            spr(art_.cross, ax, kGlassCy + bob, 13.f, 13.f, pal);
        }
        spr(art_.glass, kGlassCx, kGlassCy, 64.f, 64.f, PAL_GLASS);
    }

    if (mode_ == Mode::Aim || (mode_ == Mode::Pause && held_ == Mode::Aim)) {
        int npal = PAL_GOLD;
        if (hint_ == Kind::Swish) npal = PAL_GREEN;
        else if (hint_ == Kind::Short || hint_ == Kind::Hot || hint_ == Kind::Iron) npal = PAL_ALERT;
        float nx = kMeterX + clampf(meter_, 0.f, 1.f) * kMeterW;
        spr(art_.blot, nx, kMeterY, 3.f, 12.f, npal);
        float sweetW = (kSweetHi - kSweetLo) * kMeterW;
        float sweetX = kMeterX + (kSweetLo + kSweetHi) * 0.5f * kMeterW;
        spr(art_.blot, sweetX, kMeterY, sweetW, 8.f, PAL_GREEN);
        spr(art_.blot, kMeterX + kMeterW * 0.5f, kMeterY, kMeterW, 5.f, PAL_METER);

        float vx, vy, vz;
        if (solve(feet_ + kHand, aimZ_, vx, vy, vz)) {
            float sc = speedScale(meter_);
            vx *= sc;
            vy *= sc;
            vz *= sc;
            float x = feet_ + kHand;
            float y = kReleaseY;
            float z = 0.f;
            int pal = hint_ == Kind::Swish ? PAL_GREEN : PAL_GOLD;
            for (int i = 0; i < 16; i++) {
                float h = 0.055f;
                y += vy * h - 0.5f * kG * h * h;
                vy -= kG * h;
                x += vx * h;
                z += vz * h;
                if (y < 0.35f || x > kBoardX) break;
                spr(art_.blot, scrX(x), scrY(y) + z * 10.f, 3.f, 3.f, pal);
            }
        }
    }

    bool flip = mode_ == Mode::Leave || (mode_ == Mode::Over && won_);
    float ph = 58.f;
    float py = kFloor - ph * 0.5f;
    if (mode_ == Mode::Leave) py += std::sin(leaveT_ * 14.f) * 1.2f;
    sprM(art_.player, scrX(feet_), py, ph, PAL_YOU, flip);

    spr(art_.board, scrX(kBoardX) + 6.f, rimSy - 8.f, 50.f, 36.f, PAL_BOARD);
    spr(art_.arm, scrX(kRimX) + 16.f, rimSy + 1.f, 26.f, 5.f, PAL_IRON);
    float poleTop = rimSy + 8.f;
    float poleH = std::max(8.f, kFloor - poleTop);
    spr(art_.pole, scrX(kBoardX) + 8.f, (poleTop + kFloor) * 0.5f, 10.f, poleH, PAL_IRON);

    for (int i = 0; i < 3; i++) {
        bool live = i >= dead_;
        if (rung_ && i == dead_) live = true;
        float s = 11.f;
        if (!rung_ && i == dead_ && (mode_ == Mode::Aim || mode_ == Mode::Flight))
            s = 12.5f + std::sin(clock_ * 8.f) * 0.6f;
        float px = 262.f + float(i) * 16.f;
        if (live) sprM(art_.ball[0], px, 16.f, s, PAL_BALL);
        else spr(art_.pip, px, 16.f, 8.f, 8.f, PAL_ASH);
    }

    spr(art_.yard, 104.f, 140.f, 208.f, 62.f, PAL_COURT);
    spr(art_.sun, 286.f, 28.f, 18.f, 18.f, PAL_SUN);

    spr(art_.blot, scrX(kRimX), kFloor, 2.f, 16.f, PAL_COURT);
    spr(art_.blot, scrX(kBotX), kFloor - 1.f, 2.f, 12.f, PAL_GREEN);
    spr(art_.blot, 160.f, kFloor, 300.f, 2.f, PAL_COURT);

    float sh = clampf(16.f - ball_.y * 2.4f, 6.f, 16.f);
    spr(art_.shadow, scrX(ball_.x), kFloor + 3.f, sh, 5.f, PAL_COURT, false, true);
    spr(art_.shadow, scrX(feet_), kFloor + 4.f, 22.f, 6.f, PAL_COURT, false, true);

    char buf[48];
    int showTry = rung_ ? tryNo_ : std::min(dead_ + 1, 3);
    std::snprintf(buf, sizeof buf, "TRY %d", showTry);
    hud(1, 0, buf, PAL_GOLD);
    std::snprintf(buf, sizeof buf, "DEAD %d", dead_);
    hud(40 - int(std::strlen(buf)) - 1, 0, buf, dead_ > 0 ? PAL_ALERT : PAL_INK);

    if (mode_ == Mode::Title) {
        hudC(8, "THREE TRIES", PAL_INK);
        hudC(9, "CLEAN THROUGH THE BELL", PAL_GOLD);
        hudC(10, "THEN LEAVE", PAL_GREEN);
        hudC(25, "L-R RANGE   U-D AIM   Z SHOOT", PAL_INK);
        hudC(26, "ENTER STARTS", PAL_GREEN);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_INK);
        hudC(26, "ENTER RESUMES", PAL_INK);
    } else if (mode_ == Mode::Aim) {
        int pal = PAL_GOLD;
        if (hint_ == Kind::Swish) pal = PAL_GREEN;
        else if (hint_ == Kind::Short || hint_ == Kind::Hot || hint_ == Kind::Iron) pal = PAL_ALERT;
        hudC(3, hintWord(hint_), pal);
        hudC(26, "L-R RANGE  U-D AIM  Z SHOOT", PAL_INK);
    } else if (mode_ == Mode::Dead) {
        int left = 3 - dead_;
        std::snprintf(buf, sizeof buf, "%d LEFT", left);
        hudC(6, buf, PAL_INK);
    } else if (mode_ == Mode::Over && won_) {
        std::snprintf(buf, sizeof buf, "LEFT ON TRY %d", tryNo_);
        hudC(6, buf, PAL_GREEN);
        if (!bot_) hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Over) {
        hudC(6, "BELL SILENT", PAL_ALERT);
        hudC(8, why_, PAL_INK);
        if (!bot_) hudC(26, "ENTER PLAYS AGAIN", PAL_INK);
    } else if (mode_ == Mode::Leave) {
        hudC(6, "LEAVE THE YARD", PAL_GOLD);
    }
}

}  // namespace hoopbell

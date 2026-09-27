#include "game/turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace ferryturn {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kTurnX[3] = {78.f, 156.f, 234.f};
constexpr int kTurnDir[3] = {-1, 1, -1};
constexpr float kHalf = 18.f;
constexpr float kBand = 7.6f;
constexpr float kShore = 11.2f;
constexpr float kEnd = 312.f;
constexpr float kSlip = 2.5f;
constexpr float kTip = 0.96f;
constexpr float kCount = 0.40f;
constexpr float kArcRate = 0.90f;
constexpr float kLegLimit = 46.f;
constexpr float kCruise = 8.8f;
constexpr const char* kTurnName[3] = {"TURN 1", "TURN 2", "TURN 3"};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch hornPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.08f;
    p.op[0] = {1.f, 1.f, 0.01f, 0.22f, 0.5f, 0.3f};
    p.op[1] = {2.f, 0.2f, 0.02f, 0.24f, 0.25f, 0.2f};
    p.op[2] = {1.f, 0.05f, 0.02f, 0.3f, 0.15f, 0.25f};
    p.op[3] = {1.f, 0.f, 0.02f, 0.2f, 0.1f, 0.2f};
    p.vol = 0.18f;
    p.tone = 1400.f;
    return p;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ || mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (next_ >= 3) return 3;
    if (next_ > 0 || arc_ > 0.08f) return 2;
    return 1;
}

int Game::hullFrame() const {
    float b = clampf(list_, -0.9f, 0.9f);
    int fi = int(std::lround((b + 0.9f) / 0.3f));
    return std::clamp(fi, 0, 6);
}

void Game::showTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    toast_ = "";
    toastT_ = 0.f;
    chime_ = -1;
    next_ = 0;
    arc_ = 0.4f;
    overL_ = 0.f;
    pegT_ = 0.f;
    snap_ = true;
    x_ = kTurnX[0] - 6.f;
    lane_ = 0.4f;
    speed_ = kCruise;
    laneV_ = 0.f;
    list_ = -0.55f;
    rudder_ = 0.f;
    throttle_ = 0.55f;
    camX_ = x_ + 6.f;
    camY_ = 0.f;
    camS_ = 5.2f;
}

void Game::startRun() {
    x_ = 14.f;
    lane_ = 0.f;
    speed_ = kCruise;
    laneV_ = 0.f;
    list_ = 0.f;
    rudder_ = 0.f;
    throttle_ = 0.55f;
    overL_ = 0.f;
    pegT_ = 0.f;
    arc_ = 0.f;
    next_ = 0;
    legT_ = 0.f;
    won_ = false;
    over_ = false;
    why_ = "";
    banner_ = "";
    toast_ = "";
    toastT_ = 0.f;
    chime_ = -1;
    wakeN_ = 0;
    shake_ = 0.f;
    for (Wake& w : wakes_) w = {};
    mode_ = Mode::Cross;
    snap_ = true;
    camX_ = x_ + 8.f;
    camY_ = 0.f;
    camS_ = 5.4f;
    blip(220.f);
}

void Game::pilot(float& rudder, float& throttle) const {
    float want = 0.f;
    if (next_ < 3) {
        float dx = x_ - kTurnX[next_];
        float dir = float(kTurnDir[next_]);
        if (std::fabs(dx) <= kHalf) want = dir * 0.64f;
        else if (dx < -kHalf && dx > -(kHalf + 14.f)) {
            float u = (dx + kHalf + 14.f) / 14.f;
            want = dir * 0.64f * u;
        }
    }
    float fix = clampf(-lane_ * 0.18f - laneV_ * 0.35f, -0.28f, 0.28f);
    bool inBend = next_ < 3 && std::fabs(x_ - kTurnX[next_]) <= kHalf && arc_ < 0.98f;
    if (inBend) fix *= 0.35f;
    if (next_ >= 3) {
        float dist = kEnd - x_;
        float gain = dist < 28.f ? 0.55f : 0.28f;
        fix = clampf(-lane_ * gain - laneV_ * 0.7f, -0.85f, 0.85f);
    }
    rudder = clampf(want / 0.74f + (want - list_) * 1.05f + fix, -0.90f, 0.90f);
    float wantSpd = next_ >= 3 && (kEnd - x_) < 24.f ? 7.4f : kCruise;
    throttle = clampf(0.5f + (wantSpd - speed_) * 0.35f, 0.15f, 0.85f);
}

void Game::win() {
    if (mode_ != Mode::Cross) return;
    mode_ = Mode::Win;
    won_ = true;
    over_ = true;
    why_ = "clean";
    banner_ = "CLEAN";
    toast_ = "";
    laneV_ = 0.f;
    chime_ = 0;
    chimeT_ = 0.f;
    sys_->rumble(0.28f, 0.1f, 180);
    sys_->setLight(40, 170, 110);
}

void Game::fail(const char* why, const char* banner) {
    if (mode_ != Mode::Cross) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    why_ = why;
    banner_ = banner;
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.25f, 200);
    sys_->setLight(180, 40, 28);
    sys_->apu.noiseBurst(0.4f, 240.f, 0.35f);
}

void Game::physics(float rudder, float throttle) {
    if (mode_ != Mode::Cross) return;
    rudder_ = clampf(rudder, -1.f, 1.f);
    throttle_ = clampf(throttle, 0.f, 1.f);
    legT_ += kDt;

    float mag = std::fabs(rudder_);
    float sgn = 0.f;
    if (rudder_ > 0.02f) sgn = 1.f;
    else if (rudder_ < -0.02f) sgn = -1.f;
    bool pegged = mag > 0.97f && std::fabs(list_) > 0.58f && rudder_ * list_ > 0.f;
    if (pegged) {
        pegT_ += kDt;
        if (pegT_ > 0.7f) overL_ = std::min(1.f, overL_ + 0.40f * kDt);
    } else {
        pegT_ = std::max(0.f, pegT_ - 2.4f * kDt);
        overL_ = std::max(0.f, overL_ - 1.6f * kDt);
    }
    float set = 0.f;
    if (next_ < 3) {
        float d = std::fabs(x_ - kTurnX[next_]);
        if (d < kHalf && std::fabs(lane_) < kBand + 1.2f) set = (1.f - d / kHalf) * 0.10f * float(kTurnDir[next_]);
    }
    float urged = rudder_ * 0.74f;
    float target = urged + sgn * overL_ * 0.40f + set;
    list_ += (target - list_) * std::min(1.f, 3.3f * kDt);

    laneV_ += rudder_ * 1.15f * kDt;
    laneV_ += -laneV_ * 1.7f * kDt;
    if (next_ < 3 && std::fabs(x_ - kTurnX[next_]) < kHalf) laneV_ += float(kTurnDir[next_]) * 0.22f * kDt;
    lane_ += laneV_ * kDt;
    float wantSpd = 6.2f + throttle_ * 5.2f;
    speed_ += (wantSpd - speed_) * std::min(1.f, 0.9f * kDt);
    speed_ = clampf(speed_, 0.f, 14.f);
    x_ += speed_ * kDt;

    if (!std::isfinite(x_) || !std::isfinite(lane_) || !std::isfinite(list_) || !std::isfinite(speed_)) {
        fail("tipped", "TIPPED");
        return;
    }
    if (std::fabs(list_) > kTip) {
        fail("tipped", "TIPPED");
        return;
    }
    if (std::fabs(lane_) > kShore) {
        fail(next_ >= 3 ? "missed the end" : "missed the turn", next_ >= 3 ? "MISSED" : "NO TURN");
        return;
    }

    if (next_ < 3) {
        float dx = std::fabs(x_ - kTurnX[next_]);
        bool inL = std::fabs(lane_) <= kBand;
        float signedL = list_ * float(kTurnDir[next_]);
        if (dx <= kHalf && inL && signedL > kCount && std::fabs(list_) < kTip) {
            arc_ += signedL * kArcRate * kDt;
            if (arc_ >= 1.f) {
                toast_ = kTurnName[next_];
                toastT_ = 1.15f;
                blip(180.f + float(next_) * 40.f);
                sys_->rumble(0.16f, 0.05f, 70);
                next_++;
                arc_ = 0.f;
                overL_ = 0.f;
                pegT_ = 0.f;
            }
        } else if (x_ > kTurnX[next_] + kHalf) {
            fail("missed the turn", "NO TURN");
            return;
        }
    }

    if (x_ >= kEnd) {
        if (next_ < 3) fail("missed the turn", "NO TURN");
        else if (std::fabs(lane_) > kSlip) fail("missed the end", "MISSED");
        else win();
        return;
    }
    if (legT_ > kLegLimit) fail("missed the end", "MISSED");
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.06f);
    beep_ = 0.09f;
}

void Game::audio() {
    if (mode_ != Mode::Cross) {
        sys_->apu.noise(0.f, 400.f, false);
        sys_->apu.tone(2, 0.f, 0.f);
        return;
    }
    float wash = clampf(speed_ / 12.f, 0.f, 1.f) * 0.045f;
    sys_->apu.noise(wash, 320.f + speed_ * 18.f, false);
    bool counting = false;
    if (next_ < 3) {
        float signedL = list_ * float(kTurnDir[next_]);
        counting = std::fabs(x_ - kTurnX[next_]) <= kHalf && std::fabs(lane_) <= kBand && signedL > kCount;
    }
    if (counting) sys_->apu.tone(2, 140.f + arc_ * 220.f, 0.04f);
    else sys_->apu.tone(2, 0.f, 0.f);

    if (std::fabs(list_) > 0.84f || overL_ > 0.4f) sys_->setLight(180, 40, 28);
    else if (counting) sys_->setLight(180, 120, 30);
    else if (next_ >= 3) sys_->setLight(40, 160, 100);
    else sys_->setLight(30, 80, 150);
}

void Game::water() {
    uint16_t deep = gs::rgb4(1, 3, 7);
    uint16_t mid = gs::rgb4(2, 6, 10);
    uint16_t near = gs::rgb4(4, 9, 12);
    if (mode_ == Mode::Fail) near = lerpC(near, gs::rgb4(8, 3, 3), 0.4f);
    if (mode_ == Mode::Win) near = lerpC(near, gs::rgb4(4, 10, 7), 0.35f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.45f ? lerpC(deep, mid, t / 0.45f) : lerpC(mid, near, (t - 0.45f) / 0.55f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.setFogColor(gs::rgb4(3, 6, 8));
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float ht, int pal, bool flip, int fog) {
    if (ht < 1.2f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(ht)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(cy - s.h * 0.5f)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H + 8 || s.y + s.h < -8) return;
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    const float adv = 17.f * scale;
    x -= float(std::strlen(s)) * adv * 0.5f;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + float(i) * adv + g.w * scale * 0.5f, y, std::max(8.f, g.h * scale), pal, false);
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    water();

    float wantS = 5.5f;
    float lead = mode_ == Mode::Cross ? clampf(speed_ * 0.45f, 3.f, 8.f) : 4.f;
    float wantX = x_ + lead;
    float wantY = lane_ * 0.35f;
    if (mode_ == Mode::Title) {
        wantX = x_ + 4.f;
        wantY = 0.f;
        wantS = 5.3f;
        camX_ = wantX;
        camY_ = wantY;
        camS_ = wantS;
    } else if (snap_) {
        camX_ = wantX;
        camY_ = wantY;
        camS_ = wantS;
        snap_ = false;
    } else {
        camX_ += (wantX - camX_) * 0.12f;
        camY_ += (wantY - camY_) * 0.12f;
        camS_ += (wantS - camS_) * 0.1f;
    }
    if (shake_ > 0.f) {
        shx_ = std::sin(legT_ * 70.f) * shake_ * 4.f;
        shy_ = std::cos(legT_ * 54.f) * shake_ * 3.f;
        shake_ *= 0.9f;
        if (shake_ < 0.05f) shake_ = 0.f;
    } else {
        shx_ = shy_ = 0.f;
    }

    const float scale = camS_;
    const float ax = 150.f + shx_;
    const float ay = 112.f + shy_;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX_) * scale;
        sy = ay - (wy - camY_) * scale;
    };

    auto shore = [&](float lane, int pal) {
        for (int i = -2; i < 8; i++) {
            float wx = camX_ + float(i) * 16.f;
            float sx, sy;
            project(wx, lane, sx, sy);
            spr(art_.dock, sx, sy, 16.f, pal);
        }
    };
    shore(kShore + 1.6f, PAL_SHORE);
    shore(-kShore - 1.6f, PAL_SHORE);

    for (int i = 0; i < 3; i++) {
        float sx, sy;
        project(kTurnX[i], kBand, sx, sy);
        spr(art_.buoy, sx, sy, 28.f, PAL_BUOY, kTurnDir[i] < 0);
        project(kTurnX[i], -kBand, sx, sy);
        spr(art_.buoy, sx, sy, 28.f, PAL_BUOY, kTurnDir[i] > 0);
        project(kTurnX[i] - 6.f, 0.f, sx, sy);
        spr(art_.chev, sx, sy, 12.f, PAL_MARK, kTurnDir[i] < 0);
    }
    {
        float sx, sy;
        project(kEnd, 0.f, sx, sy);
        spr(art_.slip, sx, sy, 52.f, PAL_SLIP);
        project(kEnd - 8.f, kSlip, sx, sy);
        spr(art_.post, sx, sy, 22.f, PAL_POST);
        project(kEnd - 8.f, -kSlip, sx, sy);
        spr(art_.post, sx, sy, 22.f, PAL_POST);
    }

    for (const Wake& w : wakes_) {
        if (w.life <= 0.f) continue;
        float sx, sy;
        project(w.x, w.y, sx, sy);
        spr(art_.wake, sx, sy, 8.f + (1.f - w.life) * 10.f, PAL_WAKE, false, int((1.f - w.life) * 8.f));
    }

    for (int g = 0; g < 3; g++) {
        float gx = x_ - 20.f + float(g) * 34.f + std::sin(anim_ * 0.7f + g) * 3.f;
        float gy = 4.5f + float(g) * 1.3f + std::sin(anim_ * 1.4f + g) * 0.4f;
        float sx, sy;
        project(gx, gy, sx, sy);
        spr(art_.gull, sx, sy - 36.f, 10.f, PAL_GULL, (g & 1) != 0);
    }

    float sx, sy;
    project(x_, lane_, sx, sy);
    float bob = std::sin(anim_ * 2.2f) * 1.4f;
    spr(art_.hull[hullFrame()].img, sx, sy + bob, 46.f, PAL_HULL);

    if (mode_ == Mode::Title) {
        text("FERRY TURN", 160, 22, 1.0f, PAL_HUD);
        text("THREE TURNS", 160, 46, 0.55f, PAL_AMBER);
        text("HEEL THE BENDS", 160, 188, 0.42f, PAL_HUD);
        text("MAKE THE SLIP", 160, 204, 0.42f, PAL_GOOD);
        hudC(24, "START", PAL_AMBER);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 28, 1.0f, PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        text(banner_, 160, 24, 1.0f, PAL_BAD);
        text(why_, 160, 48, 0.42f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("CLEAN", 160, 24, 1.05f, PAL_GOOD);
        text("THREE TURNS", 160, 48, 0.48f, PAL_HUD);
    } else {
        char buf[48];
        std::snprintf(buf, sizeof buf, "TURN %d/3", std::min(next_, 3));
        hud(1, 1, buf, PAL_HUD);
        std::snprintf(buf, sizeof buf, "LIST %+4.0f", list_ * 100.f);
        hud(28, 1, buf, std::fabs(list_) > 0.8f ? PAL_BAD : PAL_HUD);
        int cells = std::clamp(int((list_ + 1.f) * 6.f), 0, 12);
        char bar[16];
        for (int i = 0; i < 13; i++) bar[i] = (i == cells) ? '|' : (i < 6 ? '-' : '-');
        bar[6] = '+';
        bar[13] = 0;
        hud(13, 2, bar, std::fabs(list_) > 0.8f ? PAL_BAD : PAL_AMBER);

        const char* line = "TO THE BEND";
        int pal = PAL_HUD;
        if (next_ >= 3) {
            float dist = std::max(0.f, kEnd - x_);
            if (dist < 30.f && std::fabs(lane_) > kSlip) {
                line = "SLIP LINE";
                pal = PAL_BAD;
            } else if (dist < 10.f && std::fabs(lane_) <= kSlip) {
                line = "THE END";
                pal = PAL_GOOD;
            } else {
                std::snprintf(buf, sizeof buf, "SLIP %3.0f M", dist);
                line = buf;
                pal = PAL_AMBER;
            }
        } else {
            float dx = kTurnX[next_] - x_;
            bool inX = std::fabs(x_ - kTurnX[next_]) <= kHalf;
            bool inL = std::fabs(lane_) <= kBand;
            float signedL = list_ * float(kTurnDir[next_]);
            if (inX && inL && signedL > kCount) {
                line = "HOLD THE HEEL";
                pal = PAL_GOOD;
            } else if (inX && inL) {
                line = kTurnDir[next_] < 0 ? "HEEL PORT" : "HEEL STARBOARD";
                pal = PAL_AMBER;
            } else if (inX) {
                line = "OUT OF THE BEND";
                pal = PAL_BAD;
            } else if (std::fabs(list_) > 0.8f) {
                line = "EASE THE HEEL";
                pal = PAL_BAD;
            } else {
                std::snprintf(buf, sizeof buf, "TURN %d   %3.0f M", next_ + 1, std::max(0.f, dx));
                line = buf;
            }
        }
        hudC(26, line, pal);
        if (toastT_ > 0.f && toast_ && toast_[0]) hudC(24, toast_, PAL_GOOD);
        else if (next_ < 3 && arc_ > 0.02f) {
            int pips = std::clamp(int(arc_ * 5.f + 0.001f), 0, 5);
            std::snprintf(buf, sizeof buf, "ARC %d/5", pips);
            hudC(25, buf, PAL_GOOD);
        } else if (overL_ > 0.25f) {
            hudC(25, "TIP", PAL_BAD);
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(3, 6, 8));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.14f, 0.22f, 0.12f);
    sys.apu.setPatch(0, hornPatch());
    if (bot_) startRun();
    else showTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) anim_ += kDt;
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (toastT_ > 0.f) toastT_ = std::max(0.f, toastT_ - kDt);
    for (Wake& w : wakes_)
        if (w.life > 0.f) w.life = std::max(0.f, w.life - kDt);

    if (chime_ >= 0) {
        static const float notes[] = {196.f, 247.f, 294.f, 392.f};
        chimeT_ += kDt;
        if (chimeT_ > 0.16f) {
            if (chime_ < 4) sys.apu.keyOn(0, notes[chime_], 0.22f);
            else sys.apu.keyOff(0);
            chime_++;
            chimeT_ = 0.f;
            if (chime_ > 8) chime_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        list_ = -0.35f - 0.22f * (0.5f + 0.5f * std::sin(anim_ * 1.1f));
        lane_ = std::sin(anim_ * 0.6f) * 0.6f;
        x_ = kTurnX[0] - 4.f;
        speed_ = kCruise;
        arc_ = 0.3f + 0.2f * (0.5f + 0.5f * std::sin(anim_ * 0.8f));
        next_ = 0;
        draw();
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            blip(240.f);
            startRun();
        } else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw();
        if (pad.pressed(gs::BTN_START)) {
            blip(200.f);
            mode_ = Mode::Cross;
        } else if (pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        audio();
        draw();
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            if (mode_ == Mode::Fail) startRun();
            else showTitle();
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            showTitle();
        }
        return;
    }

    float rudder = 0.f, throttle = throttle_;
    if (bot_) {
        pilot(rudder, throttle);
    } else {
        if (pad.down(gs::BTN_LEFT)) rudder -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) rudder += 1.f;
        if (std::fabs(pad.axisX) > 0.18f) rudder = pad.axisX;
        rudder = clampf(rudder, -1.f, 1.f);
        if (pad.down(gs::BTN_UP) || pad.accel > 0.2f) throttle = std::min(1.f, throttle + 0.4f * kDt);
        if (pad.down(gs::BTN_DOWN) || pad.brake > 0.2f) throttle = std::max(0.f, throttle - 0.5f * kDt);
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(160.f);
            draw();
            return;
        }
    }

    physics(rudder, throttle);
    if (mode_ == Mode::Cross && (sys.frame % 7) == 0) {
        wakes_[wakeN_ % 10] = {x_ - 4.2f, lane_ - list_ * 0.6f, 0.7f};
        wakeN_++;
    }
    audio();
    draw();
}

}  // namespace ferryturn

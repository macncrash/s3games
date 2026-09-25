#include "barge.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace barge {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kPi = 3.14159265f;
constexpr float kSX = 13.f;
constexpr float kSY = 18.f;
constexpr float kBase = 176.f;

constexpr float kBed = -1.60f;
constexpr float kLow = 0.f;
constexpr float kHigh = 3.20f;
constexpr float kRise = kHigh - kLow;
constexpr float kCillTop = 1.58f;
constexpr float kCillX = 17.60f;
constexpr float kDraft = 0.92f;
constexpr float kFree = 0.52f;
constexpr float kCabin = 1.18f;
constexpr float kLen = 10.6f;
constexpr float kHalf = kLen * 0.5f;
constexpr float kGateW = 1.20f;
constexpr float kGateH = 6.80f;
constexpr float kLoGateX = -0.15f;
constexpr float kUpGateX = 17.75f;
constexpr float kOpenClear = 5.25f;
constexpr float kLat = 0.86f;
constexpr float kHold = 9.30f;
constexpr float kStartX = -6.40f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

bool overlap(float a0, float a1, float b0, float b1) { return a1 > b0 && a0 < b1; }

}  // namespace

float Game::sx(float wx) const {
    float j = shake_ > 0.f ? std::sin(anim_ * 46.f) * shake_ * 3.f : 0.f;
    return 160.f + (wx - camX_) * kSX + j;
}

float Game::sy(float wh) const {
    float j = shake_ > 0.f ? std::cos(anim_ * 38.f) * shake_ * 2.f : 0.f;
    return kBase - (wh - kBed) * kSY + j;
}

float Game::bow() const { return x_ + kHalf; }
float Game::stern() const { return x_ - kHalf; }

float Game::floatH() const {
    if (stern() >= kCillX - 0.02f) return kHigh;
    return chamber_;
}

float Game::keelH() const { return floatH() - kDraft; }
float Game::roofH() const { return floatH() + kFree + kCabin; }

float Game::loBottom() const { return kOpenClear + gateLo_ * (kBed - kOpenClear); }

float Game::upBottom() const { return kOpenClear + gateUp_ * (kCillTop - kOpenClear); }

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 8, 10));
    begin();
    mode_ = bot_ ? Mode::Play : Mode::Title;
}

void Game::begin() {
    t_ = 0;
    playT_ = 0;
    leaveT_ = 0;
    settle_ = 0;
    x_ = kStartX;
    vel_ = 0;
    lat_ = 0;
    latV_ = 0;
    chamber_ = kLow;
    risen_ = 0;
    gateLo_ = 0;
    gateUp_ = 1;
    thrust_ = 0;
    rudder_ = 0;
    blipT_ = 0;
    chimeT_ = 0;
    shake_ = 0;
    over_ = false;
    won_ = false;
    why_ = "";
    phase_ = Phase::Enter;
    camX_ = std::min(8.6f, kStartX + 6.f);
}

void Game::pilot(float& thrust, float& rudder) const {
    rudder = clampf(-lat_ * 3.6f - latV_ * 1.3f, -1.f, 1.f);
    if (phase_ == Phase::Leave) {
        thrust = 0.82f;
        return;
    }
    float err = kHold - x_;
    thrust = clampf(err * 1.05f - vel_ * 1.3f, -1.f, 1.f);
    if (phase_ == Phase::Enter && x_ < kHold - 2.6f) thrust = std::max(thrust, 0.72f);
    if (bow() > kCillX - 1.40f) thrust = -1.f;
    if (phase_ != Phase::Enter && stern() < kLoGateX + kGateW + 1.15f) thrust = 1.f;
}

void Game::controls(float& thrust, float& rudder) {
    const gs::Pad& pad = sys_->pad;
    if (pad.down(gs::BTN_RIGHT) || pad.down(gs::BTN_UP)) thrust += 1.f;
    if (pad.down(gs::BTN_LEFT) || pad.down(gs::BTN_DOWN)) thrust -= 1.f;
    if (pad.accel > 0.12f) thrust += pad.accel;
    if (pad.brake > 0.12f) thrust -= pad.brake;
    if (pad.down(gs::BTN_B)) rudder += 1.f;
    if (pad.down(gs::BTN_A)) rudder -= 1.f;
    if (std::fabs(pad.axisX) > 0.12f) rudder += pad.axisX;
    thrust = clampf(thrust, -1.f, 1.f);
    rudder = clampf(rudder, -1.f, 1.f);
}

void Game::wash(float& surge, float& sway) const {
    surge = 0;
    sway = 0;
    if (phase_ == Phase::Rise || phase_ == Phase::Open) {
        float u = clampf((chamber_ - kLow) / kRise, 0.f, 1.f);
        float env = std::sin(u * kPi);
        float bias = u < 0.58f ? -1.f : 0.72f;
        surge = 2.20f * env * bias + 0.38f * env * std::sin(t_ * 2.05f);
        sway = 1.50f * env * std::sin(t_ * 1.28f + 0.4f);
    } else if (phase_ == Phase::Leave) {
        float w = std::exp(-leaveT_ * 0.55f);
        sway = 1.15f * w * std::sin(t_ * 1.7f);
        surge = 0.30f * w;
    }
}

bool Game::hurt(const char* why) {
    if (mode_ != Mode::Play) return true;
    why_ = why;
    mode_ = Mode::Fail;
    over_ = true;
    won_ = false;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.5f, 640.f, 0.35f);
    sys_->apu.tone(0, 0, 0);
    return true;
}

void Game::hazards() {
    if (mode_ != Mode::Play) return;
    float keel = keelH();
    float roof = roofH();
    if (bow() > kCillX + 0.02f && keel < kCillTop - 0.02f) {
        hurt("caught the cill");
        return;
    }
    auto gateHit = [&](float gx, float bottom, bool solid) {
        if (!solid) return false;
        if (!overlap(stern(), bow(), gx, gx + kGateW)) return false;
        if (roof <= bottom + 0.02f || keel >= bottom + kGateH - 0.02f) return false;
        return true;
    };
    if (gateHit(kLoGateX, loBottom(), gateLo_ > 0.06f)) {
        hurt("scraped the lower gate");
        return;
    }
    if (gateHit(kUpGateX, upBottom(), gateUp_ > 0.06f)) {
        hurt("scraped the upper gate");
        return;
    }
    bool narrow = bow() > kLoGateX + 0.2f && stern() < kCillX + 0.15f;
    if (narrow && std::fabs(lat_) > kLat) hurt("scraped the lock wall");
}

void Game::step() {
    float thrust = 0, rudder = 0;
    if (bot_) pilot(thrust, rudder);
    else controls(thrust, rudder);
    thrust_ = thrust;
    rudder_ = rudder;

    float surge = 0, sway = 0;
    wash(surge, sway);
    vel_ += (thrust * 3.15f - vel_ * 1.12f + surge) * kDt;
    vel_ = clampf(vel_, -2.4f, 2.8f);
    x_ += vel_ * kDt;
    latV_ += (rudder * 2.75f - latV_ * 2.45f + sway) * kDt;
    latV_ = clampf(latV_, -1.8f, 1.8f);
    lat_ += latV_ * kDt;

    if (phase_ == Phase::Enter) {
        bool box = stern() > kLoGateX + kGateW + 0.35f && bow() < kCillX - 0.28f && std::fabs(lat_) < 0.48f;
        if (box && std::fabs(vel_) < 0.50f) settle_ += kDt;
        else settle_ = 0;
        if (settle_ > 0.28f) phase_ = Phase::Shut;
    } else if (phase_ == Phase::Shut) {
        gateLo_ = std::min(1.f, gateLo_ + kDt / 0.85f);
        if (gateLo_ >= 1.f) phase_ = Phase::Rise;
    } else if (phase_ == Phase::Rise) {
        chamber_ = std::min(kHigh, chamber_ + kDt * 0.42f);
        risen_ = chamber_ - kLow;
        if (chamber_ >= kHigh - 0.001f) phase_ = Phase::Open;
    } else if (phase_ == Phase::Open) {
        gateUp_ = std::max(0.f, gateUp_ - kDt / 0.95f);
        if (gateUp_ <= 0.f) {
            phase_ = Phase::Leave;
            leaveT_ = 0;
        }
    } else if (phase_ == Phase::Leave) {
        leaveT_ += kDt;
    }

    hazards();
    if (mode_ != Mode::Play) return;

    if ((phase_ == Phase::Leave || phase_ == Phase::Open) && stern() > kCillX + 0.90f && upBottom() > roofH() - 0.04f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        risen_ = kRise;
        chimeT_ = 0;
        return;
    }
    if (playT_ > 78.f) hurt("the lock ran out of time");
}

void Game::audio() {
    gs::APU& a = sys_->apu;
    if (blipT_ > 0.f) {
        blipT_ -= kDt;
        if (blipT_ <= 0.f) a.tone(2, 0, 0);
    }
    if (mode_ == Mode::Win) {
        chimeT_ += kDt;
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        int i = std::min(3, int(chimeT_ / 0.14f));
        a.tone(1, notes[i], chimeT_ < 0.72f ? 0.07f : 0.f);
        a.tone(0, 0, 0);
        a.noise(0, 0, false);
        return;
    }
    if (mode_ != Mode::Play) {
        a.tone(0, 0, 0);
        a.tone(1, 0, 0);
        a.noise(0, 0, false);
        return;
    }
    float eng = std::fabs(thrust_);
    float vol = (eng > 0.04f || std::fabs(vel_) > 0.18f) ? 0.045f + eng * 0.03f : 0.f;
    a.tone(0, 52.f + eng * 74.f + std::fabs(vel_) * 6.f, vol);
    if (phase_ == Phase::Rise) a.noise(0.035f, 700.f + chamber_ * 180.f, false);
    else a.noise(0, 800.f, false);
    if (std::fabs(lat_) > 0.62f && phase_ != Phase::Enter) a.tone(1, 190.f, 0.028f);
    else a.tone(1, 0, 0);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    anim_ += kDt;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) {
            begin();
            mode_ = Mode::Play;
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) {
            begin();
            mode_ = Mode::Play;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    }

    if (mode_ == Mode::Play) {
        t_ += kDt;
        playT_ += kDt;
        step();
    }

    float want = 8.6f;
    if (mode_ == Mode::Title) want = 6.2f;
    else if (phase_ == Phase::Enter) want = std::min(8.6f, x_ + 6.f);
    else if (phase_ == Phase::Leave || mode_ == Mode::Win) want = std::max(8.6f, x_ - 1.2f);
    float k = mode_ == Mode::Title ? 1.f : (1.f - std::exp(-kDt * 3.4f));
    camX_ += (want - camX_) * k;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 1.8f);

    audio();
    draw();
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

const char* Game::hint() const {
    if (phase_ == Phase::Shut) return "LOWER GATE IS CLOSING";
    if (phase_ == Phase::Rise) return "HOLD HER AND FEND THE WALLS";
    if (phase_ == Phase::Open) return "UPPER GATE IS LIFTING";
    if (phase_ == Phase::Leave) return "LEAVE WITHOUT A SCRAPE";
    if (std::fabs(lat_) > 0.55f) return "LINE HER UP BEFORE THE GATE";
    if (bow() > kLoGateX && vel_ > 1.15f) return "EASE OFF";
    if (stern() > kLoGateX + kGateW && bow() < kCillX) return "STOP BETWEEN THE POSTS";
    return "ENTER THE LOCK";
}

void Game::quad(const gs::Mipped& m, float x0, float y0, float x1, float y1, int pal) {
    if (m.h < 1) return;
    if (x1 < x0) std::swap(x0, x1);
    if (y1 < y0) std::swap(y0, y1);
    float px0 = sx(x0), px1 = sx(x1);
    float py0 = sy(y1), py1 = sy(y0);
    if (px1 < -32.f || px0 > gs::SCREEN_W + 32.f || py1 < -48.f || py0 > gs::SCREEN_H + 48.f) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(px1 - px0), 1L, 1800L);
    long sh = std::clamp(std::lround(py1 - py0), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(px0), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(py0), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::mark(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -20 || cy + h * 0.5f < -20 || cx - w * 0.5f > gs::SCREEN_W + 20 || cy - h * 0.5f > gs::SCREEN_H + 20)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::screen(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (m.h < 1 || w < 1.f || h < 1.f) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 1800L);
    long sh = std::clamp(std::lround(h), 1L, 1800L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(x), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(y), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::drawFender() {
    screen(art_.bar, 78, 188, 164, 10, PAL_FENDER);
    screen(art_.bar, 74, 186, 8, 14, PAL_ALERT);
    screen(art_.bar, 238, 186, 8, 14, PAL_ALERT);
    float u = clampf(lat_ / kLat, -1.15f, 1.15f);
    float cx = 160.f + u * 74.f;
    int pal = std::fabs(lat_) > kLat * 0.72f ? PAL_ALERT : PAL_FENDER;
    mark(art_.puck, cx, 193.f, 11.f, pal);
}

void Game::drawWorld() {
    // Earlier sprites sit on top, so the barge goes in before the pound.
    float bob = std::sin(anim_ * 2.3f) * 0.03f;
    if (phase_ == Phase::Rise) bob += std::sin(anim_ * 5.1f) * 0.02f;
    float keel = keelH() + bob;
    quad(art_.boat, stern(), keel, bow(), keel + kDraft + kFree + kCabin, PAL_BOAT);

    if (std::fabs(vel_) > 0.22f) {
        float dir = vel_ >= 0.f ? 1.f : -1.f;
        float nose = dir > 0.f ? bow() : stern();
        for (int i = 0; i < 3; i++) {
            float fx = nose - dir * (0.4f + i * 0.85f);
            mark(art_.foam, sx(fx), sy(floatH()), 8.f + i, PAL_FOAM, i == 1);
        }
    }
    for (int i = 0; i < 4; i++) {
        float fx = stern() + kLen * (0.15f + 0.22f * i) + std::sin(anim_ * 1.4f + i) * 0.15f;
        mark(art_.foam, sx(fx), sy(floatH()), 7.f, PAL_FOAM);
    }
    if (phase_ == Phase::Rise) {
        for (int i = 0; i < 4; i++) {
            float yy = chamber_ + (kHigh - chamber_) * (0.25f + 0.18f * i);
            float xx = kUpGateX - 0.35f - i * 0.22f;
            quad(art_.sluice, xx, yy - 0.55f, xx + 0.18f, yy, PAL_FOAM);
        }
    }

    quad(art_.gate, kLoGateX, loBottom(), kLoGateX + kGateW, loBottom() + kGateH, PAL_GATE);
    quad(art_.gate, kUpGateX, upBottom(), kUpGateX + kGateW, upBottom() + kGateH, PAL_GATE);
    quad(art_.post, 6.55f, 4.7f, 6.95f, 6.15f, PAL_WIN);
    quad(art_.post, 11.85f, 4.7f, 12.25f, 6.15f, PAL_WIN);

    quad(art_.water, -16.f, kBed + 0.08f, kLoGateX, kLow, PAL_WATER);
    quad(art_.water, kLoGateX, kBed + 0.08f, kCillX, chamber_, PAL_WATER);
    if (chamber_ > kCillTop) quad(art_.water, kCillX, kCillTop, kUpGateX, chamber_, PAL_WATER);
    quad(art_.water, kUpGateX + kGateW, kCillTop, 34.f, kHigh, PAL_WATER);

    quad(art_.gauge, kCillX - 0.55f, kLow, kCillX - 0.28f, kHigh + 0.7f, PAL_BANNER);
    quad(art_.stone, -16.f, kBed - 1.4f, kCillX, kBed + 0.16f, PAL_STONE);
    quad(art_.stone, kCillX, kBed - 1.4f, 34.f, kCillTop, PAL_STONE);
    quad(art_.stone, kLoGateX - 0.45f, 4.55f, kLoGateX + kGateW + 0.45f, 6.7f, PAL_STONE);
    quad(art_.stone, kUpGateX - 0.35f, 4.55f, kUpGateX + kGateW + 0.45f, 6.9f, PAL_STONE);

    bool upWing = std::sin(anim_ * 4.2f) > 0.f;
    mark(art_.gull[upWing ? 0 : 1], sx(2.f + std::fmod(anim_ * 0.8f, 16.f)), sy(6.7f), 12.f, PAL_GULL);
    mark(art_.gull[upWing ? 1 : 0], sx(-4.f + std::fmod(anim_ * 0.55f, 14.f)), sy(6.35f), 10.f, PAL_GULL, true);
    mark(art_.house, sx(10.2f), sy(6.35f), 42.f, PAL_HOUSE);
    mark(art_.tree[1], sx(-3.2f), sy(6.55f), 46.f, PAL_TREE);
    mark(art_.tree[0], sx(20.4f), sy(6.45f), 40.f, PAL_TREE);
    mark(art_.cloud, sx(6.f + std::sin(anim_ * 0.15f)), sy(6.9f), 14.f, PAL_FOAM);
    mark(art_.cloud, sx(18.f), sy(6.6f), 16.f, PAL_FOAM);
    mark(art_.sun, sx(-1.5f), sy(6.55f), 22.f, PAL_SUN);
    quad(art_.grass, -16.f, 4.85f, 34.f, 5.85f, PAL_GRASS);
}

void Game::drawHud() {
    char buf[64];
    if (mode_ == Mode::Title) {
        hudC(25, "ENTER THE LOCK. RISE. LEAVE CLEAR.", PAL_BANNER);
        hudC(26, "RIGHT AHEAD   LEFT ASTERN", PAL_HUD);
        hudC(27, "Z PORT    X STARBOARD    RETURN", PAL_HUD);
        return;
    }
    hud(1, 0, "S3 BARGE", PAL_BANNER);
    int pct = int(std::lround(clampf(chamber_ / kHigh, 0.f, 1.f) * 100.f));
    std::snprintf(buf, sizeof buf, "WATER %3d%%", pct);
    hud(28, 0, buf, phase_ == Phase::Rise ? PAL_WIN : PAL_HUD);
    if (mode_ == Mode::Pause) {
        hudC(26, "RETURN CONTINUES", PAL_HUD);
        hudC(27, "ESC TO THE TITLE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(25, "LEFT THE LOCK WITHOUT A SCRAPE", PAL_WIN);
        std::snprintf(buf, sizeof buf, "ROSE %.1f M", kRise);
        hudC(26, buf, PAL_HUD);
        if (!bot_) hudC(27, "RETURN TAKES THE LOCK AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(25, why_, PAL_ALERT);
        if (!bot_) hudC(27, "RETURN TRIES THE LOCK AGAIN", PAL_HUD);
        return;
    }
    hudC(1, hint(), std::fabs(lat_) > 0.62f ? PAL_ALERT : PAL_HUD);
    hud(1, 26, "PORT", PAL_HUD);
    hud(34, 26, "STBD", PAL_HUD);
    hud(1, 27, "ARROWS DRIVE    Z PORT    X STBD", PAL_HUD);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 78) {
            float u = y / 78.f;
            int r = int(3 + 8 * u);
            int g = int(6 + 7 * u);
            int b = int(12 + 3 * u);
            c = gs::rgb4(r, g, b);
        } else {
            c = gs::rgb4(4, 4, 3);
        }
        v.lineBackdrop[y] = c;
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }

    if (mode_ == Mode::Title) mark(art_.title, 168.f, 28.f, float(art_.title.h), PAL_BANNER);
    else if (mode_ == Mode::Win) mark(art_.clear, 160.f, 34.f, float(art_.clear.h), PAL_WIN);
    else if (mode_ == Mode::Fail) {
        const gs::Mipped& ban = why_ && why_[0] == 'c' ? art_.cill : why_ && why_[0] == 't' ? art_.timeUp : art_.scraped;
        mark(ban, 160.f, 34.f, float(ban.h), PAL_ALERT);
    } else if (mode_ == Mode::Pause) mark(art_.paused, 160.f, 34.f, float(art_.paused.h), PAL_BANNER);

    drawFender();
    drawWorld();
    drawHud();
}

}  // namespace barge

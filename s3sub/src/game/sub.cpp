#include "game/sub.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <vector>

namespace sub {
namespace {

constexpr float kCruise = 80.f;
constexpr float kStartX = 180.f;
constexpr float kEnd = 4260.f;
constexpr float kPosterX = 1560.f;
constexpr float kLook = 96.f;
constexpr float kDrawH = 20.f;
constexpr float kHalfH = 10.f;
constexpr float kJet = 270.f;
constexpr float kHeavy = 44.f;
constexpr double kDt = 1.0 / 60.0;

struct Gap {
    float ceil;
    float floor;
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float bump(float x, float c, float w, float h) {
    float d = (x - c) / w;
    return h * std::exp(-d * d);
}

// Screen-space channel. y grows downward. The floor is the lethal edge.
Gap gapAt(float x) {
    float mid = 118.f + 18.f * std::sin(x * 0.0036f) + 10.f * std::sin(x * 0.0077f + 0.6f) +
                4.f * std::sin(x * 0.014f + 1.2f);
    float half = 42.f + 3.f * std::sin(x * 0.005f + 0.3f);
    float raise = bump(x, 800.f, 160.f, 16.f) + bump(x, 1550.f, 150.f, 18.f) + bump(x, 2300.f, 170.f, 16.f) +
                  bump(x, 3100.f, 150.f, 20.f) + bump(x, 3800.f, 160.f, 14.f);
    float drop = bump(x, 1150.f, 150.f, 12.f) + bump(x, 1900.f, 140.f, 14.f) + bump(x, 2700.f, 150.f, 12.f) +
                 bump(x, 3500.f, 140.f, 10.f);
    float ceil = mid - half + drop;
    float floor = mid + half - raise;
    if (floor - ceil < 64.f) {
        float m = (ceil + floor) * 0.5f;
        ceil = m - 32.f;
        floor = m + 32.f;
    }
    float u = (x - (kEnd - 200.f)) / 200.f;
    if (u > 0.f) {
        if (u > 1.f) u = 1.f;
        float e = u * u * (3.f - 2.f * u);
        ceil -= 18.f * e;
        floor += 22.f * e;
    }
    if (ceil < 10.f) {
        floor += 10.f - ceil;
        ceil = 10.f;
    }
    if (floor > 212.f) {
        ceil -= floor - 212.f;
        floor = 212.f;
    }
    if (floor < ceil + 48.f) floor = ceil + 48.f;
    return {ceil, floor};
}

float sinkAt(float x) { return bump(x, 2050.f, 180.f, 26.f) + bump(x, 3350.f, 170.f, 18.f); }

// Bias toward the ceiling so the keel stays off the silt.
float idealY(float x) {
    Gap g = gapAt(x);
    float y = g.ceil + (g.floor - g.ceil) * 0.34f;
    float lo = g.ceil + kHalfH + 8.f;
    float hi = g.floor - kHalfH - 12.f;
    if (hi < lo) return (g.ceil + g.floor) * 0.5f;
    return clampf(y, lo, hi);
}

int rockIndex(float wx, int n) {
    int i = int(std::floor(wx / 8.f)) % n;
    if (i < 0) i += n;
    return i;
}

}  // namespace

int Game::hull() const {
    int h = int(std::lround(hull_));
    if (h < 0) h = 0;
    if (h > 100) h = 100;
    return h;
}

void Game::measure() {
    int h = std::max(1, art_.sub.h);
    halfW_ = kDrawH * float(art_.sub.w) / float(h) * 0.5f;
}

void Game::showTitle() {
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    note_ = "";
    x_ = kPosterX;
    y_ = idealY(kPosterX);
    vy_ = 0.f;
    speed_ = 0.f;
    hull_ = 100.f;
    scrapes_ = 0;
    scraping_ = false;
    ceilHold_ = false;
    run_ = 0;
    t_ = 0;
    sonar_ = 0.f;
    pingCd_ = 0.f;
    minClear_ = 1000.f;
    chime_ = -1;
    motes_.clear();
}

void Game::begin() {
    mode_ = Mode::Dive;
    over_ = false;
    won_ = false;
    note_ = "";
    x_ = kStartX;
    y_ = idealY(kStartX);
    vy_ = 0.f;
    speed_ = kCruise;
    hull_ = 100.f;
    scrapes_ = 0;
    scraping_ = false;
    ceilHold_ = false;
    run_ = 0;
    sonar_ = 0.f;
    pingCd_ = 0.f;
    blowCd_ = 0.f;
    siltT_ = 0.f;
    riseT_ = 0.f;
    minClear_ = 1000.f;
    chime_ = -1;
    chimeT_ = 0;
    motes_.clear();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    measure();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(0, 0, 1));
    sys.apu.setMaster(0.75f);
    sys.apu.setEcho(0.2f, 0.4f, 0.25f);
    showTitle();
}

void Game::mote(float x, float y, float vx, float vy, int kind) {
    if (motes_.size() >= 40) return;
    Bit b;
    b.x = x;
    b.y = y;
    b.vx = vx;
    b.vy = vy;
    b.life = kind ? 0.35f : 0.55f;
    b.kind = kind;
    motes_.push_back(b);
}

void Game::blow() {
    if (blowCd_ > 0.f || mode_ != Mode::Dive) return;
    vy_ -= 52.f;
    blowCd_ = 0.65f;
    for (int i = 0; i < 4; i++) mote(float(x_) - halfW_ * 0.4f, y_ + 2.f, -20.f - i * 8.f, -18.f - i * 7.f, 0);
    sys_->apu.noiseBurst(0.18f, 900.f, 8.f);
    sys_->rumble(0.3f, 0.5f, 70);
}

void Game::gapNow(float& bot, float& top) const {
    bot = 1e9f;
    top = 1e9f;
    const float xs[5] = {-halfW_, -halfW_ * 0.5f, 0.f, halfW_ * 0.5f, halfW_};
    for (float dx : xs) {
        Gap g = gapAt(float(x_) + dx);
        bot = std::min(bot, g.floor - (y_ + kHalfH));
        top = std::min(top, (y_ - kHalfH) - g.ceil);
    }
}

void Game::look(float ahead, float& bot, float& top) const {
    bot = 1e9f;
    top = 1e9f;
    float futY = y_ + vy_ * ahead;
    float futX = float(x_) + speed_ * ahead;
    const float xs[3] = {-halfW_, 0.f, halfW_};
    for (float dx : xs) {
        Gap g = gapAt(futX + dx);
        bot = std::min(bot, g.floor - (futY + kHalfH));
        top = std::min(top, (futY - kHalfH) - g.ceil);
    }
}

void Game::pilot(float& thrust, float& throttle) {
    throttle = 0.f;
    float x = float(x_);
    float y0 = idealY(x);
    float y1 = idealY(x + 72.f);
    float slope = (y1 - y0) / 72.f;
    float wantVy = slope * speed_ + clampf((y0 - y_) * 3.6f, -46.f, 46.f);
    float ayNeed = (wantVy - vy_) * 6.f;
    thrust = clampf((ayNeed - kHeavy - sinkAt(x)) / kJet, -1.f, 1.f);

    float soon = idealY(x + 130.f);
    if (soon < y_ - 5.f) thrust = std::min(thrust, -0.5f);

    float bot = 0.f, top = 0.f;
    look(0.42f, bot, top);
    if (bot < 18.f) thrust = -1.f;
    else if (top < 12.f) thrust = 1.f;

    float botNow = 0.f, topNow = 0.f;
    gapNow(botNow, topNow);
    if (topNow < 8.f) thrust = 1.f;
    if (botNow < 12.f) thrust = -1.f;
}

void Game::controls(float& thrust, float& throttle) {
    thrust = 0.f;
    throttle = 0.f;
    if (bot_) {
        pilot(thrust, throttle);
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_UP)) thrust -= 1.f;
    if (p.down(gs::BTN_DOWN)) thrust += 1.f;
    if (p.down(gs::BTN_RIGHT)) throttle += 1.f;
    if (p.down(gs::BTN_LEFT)) throttle -= 1.f;
    if (std::fabs(p.axisX) > 0.12f) throttle = p.axisX;
    if (p.accel > 0.15f) throttle = std::max(throttle, p.accel);
    if (p.brake > 0.15f) throttle = std::min(throttle, -p.brake);
    if (p.pressed(gs::BTN_TURBO) || p.pressed(gs::BTN_B)) blow();
    if (pingCd_ <= 0.f && (p.pressed(gs::BTN_C) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_X))) {
        sonar_ = 0.8f;
        pingCd_ = 1.6f;
        sys_->apu.noiseBurst(0.22f, 2600.f, 11.f);
    }
}

void Game::physics(float thrust, float throttle) {
    run_ += kDt;
    if (sonar_ > 0.f) sonar_ -= float(kDt);
    if (pingCd_ > 0.f) pingCd_ -= float(kDt);
    if (blowCd_ > 0.f) blowCd_ -= float(kDt);
    if (siltT_ > 0.f) siltT_ -= float(kDt);
    if (riseT_ > 0.f) riseT_ -= float(kDt);

    if (bot_) {
        speed_ = kCruise;
    } else {
        float want = clampf(kCruise + throttle * 32.f, 52.f, 112.f);
        speed_ += (want - speed_) * std::min(1.f, float(kDt) * 2.8f);
    }
    float ay = thrust * kJet + kHeavy + sinkAt(float(x_));
    vy_ += ay * float(kDt);
    vy_ = clampf(vy_, -78.f, 78.f);
    y_ += vy_ * float(kDt);
    x_ += double(speed_) * kDt;

    if (vy_ < -38.f && riseT_ <= 0.f) {
        mote(float(x_) - halfW_ * 0.5f, y_ + 2.f, -14.f, -26.f, 0);
        riseT_ = 0.08f;
    }
    for (Bit& b : motes_) {
        b.x += b.vx * float(kDt);
        b.y += b.vy * float(kDt);
        b.life -= float(kDt);
    }
    motes_.erase(std::remove_if(motes_.begin(), motes_.end(), [](const Bit& b) { return b.life <= 0.f; }), motes_.end());
}

void Game::collide() {
    float bot = 0.f, top = 0.f;
    gapNow(bot, top);
    if (bot < minClear_) minClear_ = bot;

    // Sub-pixel kisses are pushed out and do not count. A real scrape does.
    if (bot < 0.f) {
        float pen = -bot;
        float into = std::max(0.f, vy_);
        y_ -= pen;
        if (vy_ > 0.f) vy_ = 0.f;
        if (pen >= 0.6f) {
            float rush = std::max(0.75f, speed_ / kCruise);
            if (!scraping_) {
                scraping_ = true;
                scrapes_++;
                hull_ -= (10.f + into * 0.4f) * rush;
                sys_->apu.noiseBurst(0.35f, 1500.f, 7.f);
                sys_->rumble(0.85f, 0.35f, 90);
            }
            hull_ -= (20.f + pen * 5.f) * rush * float(kDt);
            if (siltT_ <= 0.f) {
                mote(float(x_) + halfW_ * 0.2f, y_ + kHalfH - 1.f, -28.f, 10.f, 1);
                siltT_ = 0.05f;
            }
        } else {
            scraping_ = false;
        }
    } else {
        scraping_ = false;
    }

    gapNow(bot, top);
    if (top < 0.f) {
        float pen = -top;
        y_ += pen;
        if (vy_ < 0.f) vy_ = 0.f;
        if (pen >= 0.6f) {
            if (!ceilHold_) {
                ceilHold_ = true;
                hull_ -= 4.f;
                vy_ = std::max(vy_, 26.f);
                sys_->apu.noiseBurst(0.16f, 720.f, 9.f);
                sys_->rumble(0.25f, 0.5f, 50);
            }
            hull_ -= 7.f * float(kDt);
        }
    } else {
        ceilHold_ = false;
    }

    if (hull_ <= 0.f) {
        hull_ = 0.f;
        mode_ = Mode::Breach;
        won_ = false;
        over_ = true;
        note_ = scrapes_ > 0 ? "scraped the bottom" : "pinned on the ceiling";
        sys_->apu.noiseBurst(0.5f, 480.f, 3.5f);
        sys_->rumble(1.f, 0.45f, 220);
        return;
    }
    if (x_ >= double(kEnd) - 1e-4) {
        if (x_ > double(kEnd)) x_ = double(kEnd);
        mode_ = Mode::Clear;
        won_ = true;
        over_ = true;
        chime_ = 0;
        chimeT_ = 0;
        sys_->rumble(0.15f, 0.3f, 140);
    }
}

void Game::audio() {
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Clear) {
        static const float notes[] = {392.f, 494.f, 587.f, 784.f};
        chimeT_ += kDt;
        int step = int(chimeT_ / 0.14);
        if (step > 3) step = 3;
        a.tone(0, notes[step], step == 3 && chimeT_ > 0.7 ? 0.03f : 0.06f);
        a.tone(1, notes[step] * 2.f, 0.018f);
        a.tone(2, 0.f, 0.f);
        return;
    }
    if (mode_ == Mode::Breach) {
        a.tone(0, 42.f, 0.03f);
        a.tone(1, 0.f, 0.f);
        a.tone(2, 0.f, 0.f);
        return;
    }
    float depth = std::max(0.f, y_ - 40.f);
    float drone = 34.f + depth * 0.09f;
    float vol = mode_ == Mode::Dive ? 0.045f : 0.018f;
    a.tone(0, drone, vol);
    a.tone(1, drone * 1.51f, vol * 0.55f);
    float motor = mode_ == Mode::Dive ? 26.f + speed_ * 0.5f : 0.f;
    a.tone(2, motor, mode_ == Mode::Dive ? 0.028f : 0.f);
}

float Game::lightAt(float wx, float wy) const {
    float ambient = 0.12f;
    if (mode_ == Mode::Title || mode_ == Mode::Clear) ambient = 0.52f;
    else if (mode_ == Mode::Breach) ambient = 0.24f;
    float subX = float(x_);
    float dx = wx - (subX + halfW_ * 0.75f);
    float dy = wy - y_;
    float lamp = 0.f;
    if (dx > -28.f) {
        float dist = std::sqrt(dx * dx + dy * dy * 1.7f);
        float cone = 1.f;
        if (dx > 8.f) cone = 1.f - (std::fabs(dy) / dx) / 1.15f;
        if (cone > 0.f && dist < 220.f) lamp = cone * (1.f - dist / 220.f);
    }
    lamp *= 0.92f + 0.08f * std::sin(float(t_) * 28.f);
    if (sonar_ > 0.f) {
        float dist = std::fabs(wx - subX) * 0.7f + std::fabs(wy - y_);
        float ping = (sonar_ / 0.8f) * (1.f - dist / 380.f);
        if (ping > lamp) lamp = ping;
    }
    float gd = std::fabs(wx - kEnd);
    if (gd < 300.f) lamp = std::max(lamp, (1.f - gd / 300.f) * 0.75f);
    return clampf(std::max(lamp, ambient), 0.f, 1.f);
}

int Game::fogAt(float wx, float wy) const {
    int f = int(std::lround((1.f - lightAt(wx, wy)) * 16.f));
    if (f < 0) f = 0;
    if (f > 16) f = 16;
    return f;
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool flip) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -20 || cx - w * 0.5f > gs::SCREEN_W + 20) return;
    if (cy + h * 0.5f < -20 || cy - h * 0.5f > gs::SCREEN_H + 20) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 2000L);
    long sh = std::clamp(std::lround(h), 1L, 2000L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::sprRect(const gs::Mipped& m, float x, float y, float w, float h, int pal, int fog) {
    if (w < 1.f || h < 1.f) return;
    if (x >= gs::SCREEN_W || y >= gs::SCREEN_H || x + w <= 0.f || y + h <= 0.f) return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 2000L);
    long sh = std::clamp(std::lround(h), 1L, 2000L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(x), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(y), -2000L, 2000L));
    s.img = m.pick(std::min(float(sh), 64.f));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::label(const gs::Mipped& m, float cx, float top, int pal) {
    spr(m, cx, top + float(m.h) * 0.5f, float(m.h), pal, 0, false);
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

void Game::drawSub() {
    int pal = PAL_SUB;
    if (scraping_ && (int(t_ * 12.0) & 1) == 0) pal = PAL_HIT;
    spr(art_.sub, kLook, y_, kDrawH, pal, 0, false);
    int spin = int(std::fmod(t_ * (6.0 + double(speed_) * 0.12), 2.0));
    if (spin < 0) spin = 0;
    spr(art_.prop[spin], kLook - halfW_ + 4.f, y_ + 1.f, 8.f, pal, 0, false);
    spr(art_.glow, kLook + halfW_ + 2.f, y_ - 1.f, 7.f, PAL_LAMP, 1, false);
    if (sonar_ > 0.f) {
        float life = 1.f - sonar_ / 0.8f;
        int fog = int(clampf(life, 0.f, 1.f) * 14.f);
        spr(art_.ring, kLook + halfW_ * 0.6f, y_, 16.f + life * 64.f, PAL_LAMP, fog, false);
    } else if (mode_ == Mode::Title) {
        float p = float(0.5 + 0.5 * std::sin(t_ * 2.4));
        spr(art_.ring, kLook + halfW_ * 0.4f, y_, 22.f + p * 18.f, PAL_LAMP, 9, false);
    }
}

void Game::drawLife() {
    float cam = float(x_) - kLook;
    for (int i = 1; i < 14; i++) {
        float lx = float(i) * 300.f;
        if (lx > kEnd + 40.f) break;
        Gap g = gapAt(lx);
        float sx = lx - cam;
        int fog = fogAt(lx, g.ceil + 2.f);
        if (fog > 12) fog = 12;
        spr(art_.bulb, sx, g.ceil + 2.f, 5.f, PAL_LAMP, fog, false);
    }
    for (int i = 0; i < 6; i++) {
        float span = kEnd + 80.f;
        float fx = std::fmod(280.f + i * 690.f - float(t_) * (18.f + i * 3.f), span);
        if (fx < 0.f) fx += span;
        Gap g = gapAt(fx);
        float fy = g.ceil + (g.floor - g.ceil) * (0.40f + 0.07f * std::sin(float(t_) * 1.3f + i));
        int fog = fogAt(fx, fy);
        if (fog >= 15) continue;
        spr(art_.fish[i & 1], fx - cam, fy, (i & 1) ? 11.f : 9.f, PAL_FISH, fog, true);
    }
    for (int i = 0; i < 10; i++) {
        float mx = float(x_) - 30.f + std::fmod(float(t_) * 16.f + i * 37.f, 200.f);
        float my = y_ + std::sin(float(t_) * 2.f + i) * 7.f;
        int fog = fogAt(mx, my);
        if (fog >= 13) continue;
        spr(art_.mote, mx - cam, my, 3.f, PAL_LAMP, fog, false);
    }
    for (const Bit& b : motes_) {
        int fog = int(clampf(1.f - b.life / 0.4f, 0.f, 1.f) * 12.f);
        spr(b.kind ? art_.puff : art_.mote, b.x - cam, b.y, b.kind ? 8.f : 4.f, b.kind ? PAL_ROCK : PAL_LAMP, fog, false);
    }
}

void Game::drawGate() {
    float cam = float(x_) - kLook;
    float sx = kEnd - cam;
    if (sx < -50.f || sx > gs::SCREEN_W + 50.f) return;
    Gap g = gapAt(kEnd);
    float gh = std::min(120.f, g.floor - g.ceil);
    float dist = std::fabs(kEnd - float(x_));
    int fog = 3;
    if (dist > 30.f) fog = int(std::min(11.f, dist / 48.f));
    spr(art_.gate, sx, (g.ceil + g.floor) * 0.5f, gh, PAL_GATE, fog, false);
}

void Game::drawRocks() {
    float cam = float(x_) - kLook;
    for (int col = 0; col < 40; col++) {
        float x0 = float(col * 8);
        float wx = cam + x0 + 4.f;
        Gap g = gapAt(wx);
        int top = int(std::floor(g.ceil));
        int lip = int(std::ceil(g.floor));
        if (top > 0) {
            int h = std::min(top, gs::SCREEN_H);
            sprRect(art_.ceil[rockIndex(wx, 3)], x0, 0.f, 8.f, float(h), PAL_CEIL, fogAt(wx, g.ceil));
        }
        if (lip < gs::SCREEN_H) {
            int y = std::max(0, lip);
            sprRect(art_.rock[rockIndex(wx, 3)], x0, float(y), 8.f, float(gs::SCREEN_H - y), PAL_ROCK, fogAt(wx, g.floor));
        }
    }
}

void Game::drawHud() {
    char buf[48];
    if (mode_ == Mode::Title) {
        label(art_.title, 160.f, 6.f, PAL_HUD);
        label(art_.tag, 160.f, 168.f, PAL_WARN);
        hudC(23, "REACH THE LOCK LIGHT", PAL_DIM);
        hudC(24, "UP RISE   DOWN DIVE", PAL_HUD);
        hudC(25, "SPACE BLOW   C SONAR", PAL_HUD);
        hudC(26, "LEFT SLOW   RIGHT FAST", PAL_DIM);
        if ((int(t_ * 2.0) & 1) == 0) hudC(27, "PRESS START", PAL_WIN);
        return;
    }
    if (mode_ == Mode::Pause) {
        label(art_.paused, 160.f, 92.f, PAL_HUD);
        hudC(18, "START CONTINUES", PAL_DIM);
        return;
    }
    if (mode_ == Mode::Clear) {
        label(art_.clear, 160.f, 8.f, PAL_WIN);
        std::snprintf(buf, sizeof buf, "HULL %d", hull());
        hudC(23, buf, PAL_HUD);
        hudC(24, scrapes_ == 0 ? "NEVER SCRAPED THE BOTTOM" : "THE KEEL KISSED SILT", scrapes_ == 0 ? PAL_WIN : PAL_WARN);
        if (scrapes_ > 0) {
            std::snprintf(buf, sizeof buf, "SCRAPES %d", scrapes_);
            hudC(25, buf, PAL_WARN);
        }
        std::snprintf(buf, sizeof buf, "TIME %.1f", run_);
        hudC(26, buf, PAL_DIM);
        if (!bot_) hudC(27, "START DIVES AGAIN", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Breach) {
        label(art_.breach, 160.f, 8.f, PAL_ALERT);
        hudC(23, note_ && note_[0] ? note_ : "HULL OPEN", PAL_ALERT);
        hudC(24, "THE CHANNEL KEEPS THE KEEL", PAL_WARN);
        if (!bot_) hudC(27, "START DIVES AGAIN", PAL_HUD);
        return;
    }

    hud(1, 0, "S3 SUB", PAL_DIM);
    double u = (x_ - double(kStartX)) / double(kEnd - kStartX);
    int pct = int(std::lround(u * 100.0));
    if (pct < 0) pct = 0;
    if (pct > 100) pct = 100;
    std::snprintf(buf, sizeof buf, "WAY %3d", pct);
    hud(31, 0, buf, PAL_HUD);

    int hp = hull();
    std::snprintf(buf, sizeof buf, "HULL %3d", hp);
    hud(1, 1, buf, hp < 35 ? PAL_ALERT : PAL_HUD);
    char bar[12];
    int blocks = hp / 10;
    if (hp > 0 && blocks == 0) blocks = 1;
    for (int i = 0; i < 10; i++) bar[i] = i < blocks ? '#' : '-';
    bar[10] = 0;
    hud(12, 1, bar, hp < 35 ? PAL_ALERT : PAL_WIN);

    float bot = 0.f, top = 0.f;
    gapNow(bot, top);
    int gap = int(std::floor(bot));
    if (gap < 0) gap = 0;
    std::snprintf(buf, sizeof buf, bot < 14.f ? "BOTTOM %d" : "FLOOR %d", gap);
    hud(1, 2, buf, bot < 14.f ? PAL_ALERT : PAL_HUD);
    if (pct >= 85) hud(33, 2, "LOCK", PAL_WIN);

    if (sinkAt(float(x_)) > 12.f) hud(1, 3, "CURRENT DOWN", PAL_WARN);
    else {
        std::snprintf(buf, sizeof buf, "SPD %d", int(std::lround(speed_)));
        hud(1, 3, buf, PAL_HUD);
    }
    if (pingCd_ <= 0.f) hud(32, 3, "SONAR", PAL_DIM);
    else hud(33, 3, "PING", PAL_WARN);

    if (bot < 14.f) hudC(26, "DON'T SCRAPE THE BOTTOM", PAL_ALERT);
    else if (top < 8.f) hudC(26, "OFF THE CEILING", PAL_WARN);
    else if (run_ < 4.0) hudC(26, "RIDE HIGH  WATCH THE FLOOR", PAL_DIM);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    uint16_t bg = gs::rgb4(0, 0, 1);
    if (mode_ == Mode::Title || mode_ == Mode::Clear) bg = gs::rgb4(0, 1, 2);
    else if (mode_ == Mode::Breach) bg = gs::rgb4(2, 0, 0);
    v.setFogColor(bg);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineBackdrop[y] = bg;
        v.lineFog[y] = 0;
    }
    if (mode_ == Mode::Title || mode_ == Mode::Clear || mode_ == Mode::Breach || mode_ == Mode::Pause) {
        // Banners sit above the boat. Rocks are last so they stay behind.
    }
    if (mode_ == Mode::Title) {
        drawSub();
        drawLife();
        drawRocks();
        drawHud();
        return;
    }
    drawSub();
    drawLife();
    drawGate();
    drawRocks();
    drawHud();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += kDt;

    if (mode_ == Mode::Title) {
        bool go = (bot_ && t_ >= 0.40) || (!bot_ && sys.pad.anyPressed());
        draw();
        audio();
        if (go) begin();
        return;
    }
    if (mode_ == Mode::Pause) {
        if (sys.pad.pressed(gs::BTN_START)) mode_ = Mode::Dive;
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Clear || mode_ == Mode::Breach) {
        if (!bot_ && sys.pad.anyPressed()) begin();
        if (mode_ != Mode::Dive) {
            draw();
            audio();
            return;
        }
    }
    if (mode_ == Mode::Dive && !bot_ && sys.pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        audio();
        return;
    }

    float thrust = 0.f, throttle = 0.f;
    controls(thrust, throttle);
    physics(thrust, throttle);
    collide();
    float bot = 0.f, top = 0.f;
    if (mode_ == Mode::Dive) {
        gapNow(bot, top);
        if (bot < 14.f) sys.setLight(180, 80, 20);
        else sys.setLight(16, 48, 130);
    } else if (mode_ == Mode::Clear) {
        sys.setLight(40, 180, 130);
    } else if (mode_ == Mode::Breach) {
        sys.setLight(180, 24, 20);
    }
    draw();
    audio();
}

}  // namespace sub

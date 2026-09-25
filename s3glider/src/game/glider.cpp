#include "game/glider.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "version.h"

namespace glider {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float G = 9.81f;
constexpr float kSpoilSink = 6.2f;
constexpr float kSpoilDrag = 2.6f;
constexpr float kTreeLine = 210.f;
constexpr float kField0 = 2140.f;
constexpr float kField1 = 3140.f;
constexpr float kCanopy = 14.f;
constexpr float kCruise = 46.f;
constexpr float kEnd = 3600.f;

struct Knot {
    float x, y;
};

constexpr Knot kGround[] = {
    {0.f, 72.f},   {190.f, 72.f},  {560.f, 182.f}, {750.f, 182.f}, {930.f, 150.f},
    {1300.f, 260.f}, {1480.f, 260.f}, {2140.f, 64.f}, {3140.f, 64.f}, {3520.f, 460.f},
};
constexpr int kGroundN = int(sizeof(kGround) / sizeof(kGround[0]));

float ground(float x) {
    if (x <= kGround[0].x) return kGround[0].y;
    if (x >= kGround[kGroundN - 1].x) return kGround[kGroundN - 1].y;
    for (int i = 0; i < kGroundN - 1; i++) {
        if (x <= kGround[i + 1].x) {
            float u = (x - kGround[i].x) / (kGround[i + 1].x - kGround[i].x);
            return kGround[i].y + (kGround[i + 1].y - kGround[i].y) * u;
        }
    }
    return kGround[kGroundN - 1].y;
}

float slopeAt(float x) {
    const float d = 18.f;
    return (ground(x + d) - ground(x - d)) / (2.f * d);
}

float canopy(float x) {
    if (x < kTreeLine) return 0.f;
    if (x >= kField0 && x <= kField1) return 0.f;
    return kCanopy;
}

float ridgeLift(float x, float y) {
    float agl = y - ground(x);
    if (agl < 3.f || agl > 120.f) return 0.f;
    float slope = slopeAt(x);
    float band = 1.f;
    if (agl > 55.f) band = std::clamp((120.f - agl) / 65.f, 0.f, 1.f);
    if (slope > 0.04f) {
        float low = std::clamp(agl / 28.f, 0.45f, 1.f);
        return std::clamp(slope * 36.f * band * low, 0.f, 9.5f);
    }
    if (slope < -0.08f) {
        float lee = std::clamp((-slope - 0.08f) * 14.f, 0.f, 3.2f);
        if (agl > 70.f) lee *= 0.35f;
        return -lee;
    }
    return 0.f;
}

float desiredAgl(float x) {
    if (x < 240.f) return 42.f;
    if (x < kField0) {
        float u = std::clamp((x - 240.f) / 200.f, 0.f, 1.f);
        return 42.f + (kCruise - 42.f) * u;
    }
    float u = std::clamp((x - kField0) / 520.f, 0.f, 1.f);
    float s = u * u * (3.f - 2.f * u);
    return kCruise + (0.4f - kCruise) * s;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch tonePatch() {
    gs::FMPatch p;
    p.alg = 7;
    p.vol = 0.2f;
    p.tone = 1600.f;
    p.op[0] = {1.f, 1.f, 0.012f, 0.18f, 0.75f, 0.16f};
    p.op[1] = {2.f, 0.18f, 0.02f, 0.22f, 0.4f, 0.18f};
    p.op[2] = {3.f, 0.06f, 0.02f, 0.25f, 0.25f, 0.2f};
    p.op[3] = {1.f, 0.0f, 0.02f, 0.2f, 0.2f, 0.2f};
    return p;
}

}  // namespace

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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    float top = feet ? cy - s.h : cy - s.h * 0.5f;
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float top, float w, float h, int pal) {
    if (w < 1.5f || h < 1.5f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::clamp(long(std::lround(cx - s.w * 0.5f)), -8000L, 8000L));
    s.y = int16_t(std::clamp(long(std::lround(top)), -8000L, 8000L));
    if (s.x > gs::SCREEN_W + 8 || s.x + s.w < -8 || s.y > gs::SCREEN_H || s.y + s.h < 0) return;
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::blip(bool high) {
    sys_->apu.tone(1, high ? 740.f : 420.f, 0.06f);
    beep_ = 0.06f;
}

void Game::layoutWorld() {
    trees_.clear();
    clouds_.clear();
    birds_.clear();
    for (float x = 230.f; x < 3480.f; x += 24.f) {
        if (canopy(x) <= 0.f) continue;
        Mark m;
        m.x = x;
        m.y = ground(x);
        m.h = 15.2f + float(int(x) % 5) * 0.7f;
        m.kind = (int(x) / 24) & 1;
        trees_.push_back(m);
    }
    for (int i = 0; i < 9; i++) {
        Mark m;
        m.x = 120.f + i * 340.f;
        m.y = ground(m.x) + 96.f + float(i % 3) * 22.f;
        m.h = 18.f + float(i % 3) * 4.f;
        m.kind = 0;
        clouds_.push_back(m);
    }
    for (int i = 0; i < 5; i++) {
        Mark m;
        m.x = 380.f + i * 280.f;
        m.y = ground(m.x) + 38.f + float(i % 2) * 8.f;
        m.h = 3.2f;
        m.kind = i & 1;
        birds_.push_back(m);
    }
    sockX_ = kField0 + 46.f;
    barnX_ = kField0 + 120.f;
}

void Game::launch() {
    x_ = 52.f;
    y_ = ground(x_) + 48.f;
    v_ = 22.5f;
    pitch_ = -0.07f;
    spoil_ = 0.f;
    spoilWas_ = false;
    wind_ = ridgeLift(x_, y_);
    vy_ = v_ * std::sin(pitch_) + wind_;
    agl_ = y_ - ground(x_);
    shake_ = 0.f;
    mode_ = Mode::Fly;
    result_[0] = 0;
}

void Game::newGame() {
    score_ = 0;
    lives_ = 3;
    won_ = false;
    over_ = false;
    fanStep_ = -1;
    launch();
}

void Game::succeed() {
    if (mode_ != Mode::Fly) return;
    std::snprintf(result_, sizeof result_, "landed in the field");
    int soft = int(std::max(0.f, (4.6f + vy_) * 420.f));
    int speed = int(std::max(0.f, 16.f - std::fabs(v_ - 22.f)) * 35.f);
    score_ += 2500 + int(x_) + soft + speed;
    won_ = true;
    over_ = true;
    mode_ = Mode::Victory;
    y_ = ground(x_);
    agl_ = 0.f;
    if (pitch_ < -0.04f) pitch_ = -0.04f;
    fanStep_ = 0;
    fanT_ = 0.f;
    sys_->rumble(0.18f, 0.08f, 90);
    sys_->setLight(40, 170, 70);
    sys_->apu.noise(0.f, 1000.f, false);
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Fly) return;
    std::snprintf(result_, sizeof result_, "%s", why);
    lives_--;
    shake_ = 1.f;
    sys_->rumble(0.55f, 0.35f, 160);
    sys_->setLight(180, 30, 20);
    sys_->apu.noiseBurst(0.5f, 700.f, 0.32f);
    sys_->apu.tone(2, 0.f, 0.f);
    if (bot_ || lives_ <= 0) {
        over_ = true;
        won_ = false;
        mode_ = Mode::Over;
    } else {
        mode_ = Mode::Dead;
        deadT_ = 0.f;
    }
}

void Game::steer(float& nose, float& spoil) {
    spoil = 0.f;
    float vx = std::max(8.f, v_ * std::cos(pitch_));
    float gRise = slopeAt(x_) * vx;
    float aglRate = vy_ - gRise;
    float targetAglRate;
    bool field = x_ >= kField0 && x_ <= kField1;
    if (!field) {
        float err = desiredAgl(x_) - agl_;
        targetAglRate = std::clamp(err * 0.48f - aglRate * 0.22f, -5.5f, 3.5f);
        if (canopy(x_) > 0.f && agl_ < kCanopy + 12.f) targetAglRate = std::max(targetAglRate, 2.2f);
        if (v_ < 17.5f && agl_ > kCanopy + 14.f) targetAglRate = std::min(targetAglRate, -1.1f);
        if (v_ > 31.f) targetAglRate = std::max(targetAglRate, 0.8f);
    } else {
        float flareAt = kField1 - 360.f;
        if (x_ < flareAt) {
            float time = std::max(0.5f, (flareAt - x_) / vx);
            targetAglRate = std::clamp(-(agl_ - 9.f) / time, -6.f, -0.2f);
        } else {
            float time = std::max(0.45f, (kField1 - 50.f - x_) / vx);
            float want = std::clamp(-(agl_ - 0.45f) / time, -5.f, -0.35f);
            if (agl_ < 8.f) want = std::max(want, -1.25f);
            if (agl_ < 3.2f) want = std::max(want, -0.62f);
            targetAglRate = want;
        }
    }
    float lift = ridgeLift(x_, y_);
    float targetVy = gRise + targetAglRate;
    float need = targetVy - lift;
    float vv = std::max(v_, 12.f);
    if (!(field && agl_ < 10.f) && need < -0.42f * vv) {
        float deficit = -0.42f * vv - need;
        spoil = std::clamp(deficit / kSpoilSink, 0.f, 1.f);
    }
    if (field && agl_ < 9.f) spoil = 0.f;
    if (canopy(x_) > 0.f && agl_ < kCanopy + 12.f) spoil = 0.f;
    need = targetVy - lift + spoil * kSpoilSink;
    float sinP = std::clamp(need / vv, -0.55f, 0.50f);
    float targetPitch = std::asin(sinP);
    nose = std::clamp((targetPitch - pitch_) / 0.16f, -1.f, 1.f);
}

void Game::fly(float nose, float spoil) {
    spoil_ = std::clamp(spoil, 0.f, 1.f);
    if (std::fabs(nose) < 0.08f) pitch_ += (-0.07f - pitch_) * 0.85f * DT;
    else pitch_ += nose * 1.35f * DT;
    pitch_ = std::clamp(pitch_, -0.62f, 0.55f);
    if (v_ < 12.5f) {
        float m = (12.5f - v_) / 12.5f;
        pitch_ += (-0.55f - pitch_) * m * 3.f * DT;
        pitch_ = std::clamp(pitch_, -0.62f, 0.55f);
    }

    float sp = std::sin(pitch_);
    float cp = std::cos(pitch_);
    float drag = 0.00155f * v_ * v_ + 0.35f * pitch_ * pitch_ * v_;
    drag += spoil_ * kSpoilDrag;
    if (v_ < 12.5f) drag += (12.5f - v_) * 0.45f;
    v_ += (-G * sp - drag) * DT;
    v_ = std::clamp(v_, 6.f, 46.f);

    wind_ = ridgeLift(x_, y_);
    float vy = v_ * sp + wind_ - spoil_ * kSpoilSink;
    float vx = std::max(7.f, v_ * cp);
    bool clear = canopy(x_) <= 0.f;
    float agl = y_ - ground(x_);
    if (clear && agl > 0.f && agl < 6.f && vy < 0.f) {
        float ge = (6.f - agl) / 6.f * 0.4f;
        vy *= (1.f - ge);
    }
    vy_ = vy;
    x_ += vx * DT;
    y_ += vy * DT;
    agl_ = y_ - ground(x_);
    if (!std::isfinite(x_) || !std::isfinite(y_) || !std::isfinite(v_)) {
        fail("lost the air");
        return;
    }

    bool field = x_ >= kField0 && x_ <= kField1 && canopy(x_) <= 0.f;
    if (field && agl_ <= 1.0f) {
        if (vy_ >= -4.6f && v_ >= 11.f) succeed();
        else fail(v_ < 11.f ? "too slow" : "landed too hard");
        return;
    }
    if (canopy(x_) > 0.f && agl_ <= canopy(x_)) {
        fail("in the trees");
        return;
    }
    if (agl_ <= 0.35f) {
        fail(x_ < kTreeLine ? "hit the ground" : "hit the ridge");
        return;
    }
    if (x_ > kEnd) fail("missed the field");
}

void Game::sky() {
    uint16_t zen = gs::rgb4(3, 6, 13);
    uint16_t mid = gs::rgb4(6, 11, 15);
    uint16_t hor = gs::rgb4(15, 13, 10);
    if (wind_ > 2.f) hor = lerpC(hor, gs::rgb4(12, 15, 14), 0.35f);
    else if (wind_ < -1.f) hor = lerpC(hor, gs::rgb4(12, 10, 12), 0.4f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        sys_->vdp.lineBackdrop[y] = t < 0.62f ? lerpC(zen, mid, t / 0.62f) : lerpC(mid, hor, (t - 0.62f) / 0.38f);
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.setFogColor(gs::rgb4(9, 11, 13));
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
}

void Game::draw(float camX, float camY, float pitch, bool craft) {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    sky();

    float agl = std::max(6.f, camY - ground(camX));
    float scale = 2.25f;
    if (agl * scale > 156.f) scale = 156.f / agl;
    if (scale < 0.42f) scale = 0.42f;
    float shx = 0.f, shy = 0.f;
    if (shake_ > 0.f) {
        shx = std::sin(t_ * 47.f) * 5.f * shake_;
        shy = std::cos(t_ * 39.f) * 3.f * shake_;
    }
    const float ax = 112.f + shx;
    const float ay = 70.f + shy;
    auto project = [&](float wx, float wy, float& sx, float& sy) {
        sx = ax + (wx - camX) * scale;
        sy = ay - (wy - camY) * scale;
    };

    auto banner = [&]() {
        if (mode_ == Mode::Title) {
            text("S3 GLIDER", 160, 28, 1.15f, PAL_HUD);
            text("RIDGE LIFT", 160, 52, 0.72f, PAL_AMBER);
        } else if (mode_ == Mode::Pause) {
            text("PAUSE", 160, 36, 1.2f, PAL_HUD);
        } else if (mode_ == Mode::Dead || mode_ == Mode::Over) {
            text(result_, 160, 40, 0.85f, PAL_BAD);
        } else if (mode_ == Mode::Victory) {
            text("ON THE FIELD", 160, 34, 0.9f, PAL_GOOD);
        }
    };
    banner();

    if (craft) {
        int fi = int(std::lround((0.46f - pitch) / 0.23f));
        fi = std::clamp(fi, 0, 4);
        float gh = 38.f * std::clamp(scale / 2.15f, 0.62f, 1.25f);
        float sx, sy;
        float wheel = gh * 16.f / 140.f;
        project(camX, camY + wheel / scale, sx, sy);
        spr(art_.ship[fi], sx, sy, gh, PAL_SHIP, false);
        if (mode_ == Mode::Victory || mode_ == Mode::Dead || mode_ == Mode::Over) {
            spr(art_.puff, sx - 10.f, sy + 8.f, 16.f + shake_ * 10.f, PAL_SKY, false);
        }
    }

    if (wind_ > 1.3f && (mode_ == Mode::Fly || mode_ == Mode::Title || mode_ == Mode::Pause)) {
        for (int i = 0; i < 4; i++) {
            float ph = std::fmod(t_ * 14.f + i * 8.f, 32.f);
            float wx = camX - 16.f + i * 9.f;
            float wy = ground(wx) + 16.f + ph;
            if (wy > camY + 30.f) continue;
            float sx, sy;
            project(wx, wy, sx, sy);
            spr(art_.chevron, sx, sy, 12.f, PAL_LIFT, false, 2);
        }
    }

    int flap = int(t_ * 6.f) & 1;
    for (const Mark& b : birds_) {
        float sx, sy;
        float bx = b.x + std::sin(t_ * 0.4f + b.x) * 6.f;
        project(bx, b.y + std::sin(t_ * 1.3f + b.x) * 1.5f, sx, sy);
        spr(art_.bird[flap], sx, sy, b.h * scale, PAL_BIRD, false, 1);
    }

    for (const Mark& tr : trees_) {
        float sx, sy;
        project(tr.x, tr.y, sx, sy);
        if (sx < -40 || sx > gs::SCREEN_W + 40) continue;
        const gs::Mipped& img = tr.kind ? art_.broad : art_.pine;
        spr(img, sx, sy, tr.h * scale, PAL_TREE, false, 0, true);
    }

    if (sockX_ > 0.f) {
        float sx, sy;
        project(sockX_, ground(sockX_), sx, sy);
        int fr = int(t_ * 3.f) % 3;
        if (fr < 0) fr = 0;
        spr(art_.sock[fr], sx, sy, 11.f * scale, PAL_PROP, false, 0, true);
        project(kField0 + 8.f, ground(kField0), sx, sy);
        spr(art_.gate, sx, sy - 2.f, 5.f * std::max(scale, 0.8f), PAL_PROP, false);
    }
    if (barnX_ > 0.f) {
        float sx, sy;
        project(barnX_, ground(barnX_), sx, sy);
        spr(art_.barn, sx, sy, 9.f * scale, PAL_PROP, false, 0, true);
    }

    if (craft && agl < 100.f) {
        float sx, sy;
        project(camX - 4.f, ground(camX), sx, sy);
        float sh = (11.f + agl * 0.05f) * std::clamp(scale, 0.5f, 2.2f);
        spr(art_.shade, sx, sy, std::max(3.f, sh * 0.28f), PAL_FAR, false);
    }

    float left = camX - (ax + 30.f) / scale;
    float right = camX + (gs::SCREEN_W - ax + 30.f) / scale;
    float step = 8.f;
    if ((right - left) / step > 58.f) step = (right - left) / 58.f;
    float start = std::floor(left / step) * step;
    for (float wx = start; wx < right; wx += step) {
        float sy = ay - (ground(wx) - camY) * scale;
        if (sy > gs::SCREEN_H + 4) continue;
        float h = gs::SCREEN_H + 12.f - sy;
        if (h < 2.f) continue;
        if (h > 640.f) h = 640.f;
        float sx = ax + (wx - camX) * scale;
        float sl = slopeAt(wx);
        const gs::Mipped* img = &art_.grass;
        int pal = PAL_GRASS;
        if (wx >= kField0 && wx <= kField1) {
            img = &art_.field;
            pal = PAL_FIELD;
        } else if (sl > 0.12f) {
            img = &art_.rock;
            pal = PAL_ROCK;
        }
        float sw = step * scale + 2.f;
        sprBox(*img, sx, sy, sw, h, pal);
    }

    for (int i = 0; i < 5; i++) {
        float sx = std::fmod(30.f + i * 160.f - camX * 0.18f * scale, 820.f);
        if (sx < -80.f) sx += 820.f;
        spr(art_.hill, sx, 132.f, 48.f, PAL_FAR, false, 7);
    }
    for (const Mark& c : clouds_) {
        float sx, sy;
        project(c.x - camX * 0.15f, c.y, sx, sy);
        sx = std::fmod(sx + 400.f, 520.f) - 40.f;
        spr(art_.cloud, sx, 36.f + float(int(c.x) % 3) * 14.f, c.h, PAL_SKY, false, 3);
    }
    spr(art_.sun, 276, 30, 22, PAL_SKY, false, 0);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(16, "UP DOWN  PITCH", PAL_HUD);
        hudC(18, "Z SPACE  SPOILERS", PAL_HUD);
        hudC(20, "RIDE THE WINDWARD SLOPE", PAL_HUD);
        hudC(21, "STAY ABOVE THE TREES", PAL_HUD);
        hudC(22, "LAND IN THE GOLD FIELD", PAL_AMBER);
        if ((sys_->frame / 30) % 2 == 0) hudC(25, "PRESS START", PAL_GOOD);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Fly || mode_ == Mode::Pause) {
        std::snprintf(buf, sizeof buf, "SPD %02.0f", v_);
        hud(1, 1, buf, v_ < 14.f ? PAL_BAD : PAL_HUD);
        std::snprintf(buf, sizeof buf, "AGL %03.0f", std::max(0.f, agl_));
        int ap = PAL_HUD;
        if (canopy(camX) > 0.f && agl_ < kCanopy + 8.f) ap = PAL_BAD;
        hud(11, 1, buf, ap);
        std::snprintf(buf, sizeof buf, "VAR %+4.1f", vy_);
        hud(22, 1, buf, vy_ > 0.5f ? PAL_GOOD : vy_ < -1.6f ? PAL_BAD : PAL_HUD);
        if (camX < kField0) {
            int dist = std::max(0, int(kField0 - camX));
            std::snprintf(buf, sizeof buf, "FIELD %dm", dist);
            hud(28, 26, buf, PAL_AMBER);
        } else if (camX <= kField1) {
            hud(30, 26, "FIELD", PAL_GOOD);
        }
        if (spoil_ > 0.2f) hud(1, 26, "SPOILERS", PAL_AMBER);
        if (wind_ > 1.8f) hud(1, 3, "RIDGE", PAL_GOOD);
        else if (wind_ < -1.2f) hud(1, 3, "SINK", PAL_BAD);
        if (canopy(camX) > 0.f && agl < kCanopy + 7.f) hudC(14, "TREES", PAL_BAD);
        else if (camX >= kField0 && agl > 28.f && (sys_->frame / 30) % 2 == 0) hudC(14, "GET DOWN", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hud(1, 27, buf, lives_ > 1 ? PAL_HUD : PAL_BAD);
        if (mode_ == Mode::Pause) {
            hudC(18, "START  RESUME", PAL_HUD);
            hudC(20, "ESC    TITLE", PAL_HUD);
        }
    } else if (mode_ == Mode::Dead) {
        std::snprintf(buf, sizeof buf, "LIVES %d", lives_);
        hudC(16, buf, PAL_HUD);
        hudC(20, "START", PAL_HUD);
    } else if (mode_ == Mode::Over) {
        hudC(16, "THE TREES KEEP THE RIDGE", PAL_HUD);
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(18, buf, PAL_AMBER);
        hudC(22, "START", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        std::snprintf(buf, sizeof buf, "SCORE %d", score_);
        hudC(16, buf, PAL_AMBER);
        hudC(18, "NOT THE TREES", PAL_GOOD);
        hudC(22, "START", PAL_HUD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    layoutWorld();
    sys.vdp.setFogColor(gs::rgb4(9, 11, 13));
    sys.apu.setMaster(0.82f);
    sys.apu.setEcho(0.14f, 0.22f, 0.14f);
    sys.apu.setPatch(0, tonePatch());
    if (bot_) newGame();
    else {
        mode_ = Mode::Title;
        won_ = false;
        over_ = false;
        lives_ = 3;
        score_ = 0;
        result_[0] = 0;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) t_ += DT;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(1, 0.f, 0.f);
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 0.8f);

    if (fanStep_ >= 0) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f};
        fanT_ += DT;
        if (fanT_ > 0.14f) {
            if (fanStep_ < 4) sys.apu.keyOn(0, notes[fanStep_], 0.2f);
            else sys.apu.keyOff(0);
            fanStep_++;
            fanT_ = 0.f;
            if (fanStep_ > 7) fanStep_ = -1;
        }
    }

    if (!bot_ && mode_ == Mode::Title) {
        float cx = 260.f + std::fmod(t_ * 22.f, 860.f);
        float cy = ground(cx) + 44.f + std::sin(t_ * 0.8f) * 4.f;
        float p = std::sin(t_ * 0.6f) * 0.16f;
        wind_ = ridgeLift(cx, cy);
        draw(cx, cy, p, true);
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            newGame();
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
        return;
    }

    if (mode_ == Mode::Pause) {
        draw(x_, y_, pitch_, true);
        if (pad.pressed(gs::BTN_START)) {
            blip(true);
            mode_ = Mode::Fly;
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
        }
        return;
    }

    if (mode_ == Mode::Dead) {
        deadT_ += DT;
        draw(x_, y_, pitch_, true);
        if (pad.pressed(gs::BTN_START) || deadT_ > 1.15f) launch();
        return;
    }

    if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        draw(x_, y_, pitch_, true);
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
        sys.apu.tone(2, 0.f, 0.f);
        return;
    }

    float nose = 0.f, spoil = 0.f;
    if (bot_) {
        steer(nose, spoil);
    } else {
        if (pad.down(gs::BTN_UP)) nose += 1.f;
        if (pad.down(gs::BTN_DOWN)) nose -= 1.f;
        nose = std::clamp(nose, -1.f, 1.f);
        if (pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_C) || pad.down(gs::BTN_TURBO) ||
            pad.accel > 0.25f || pad.brake > 0.25f)
            spoil = 1.f;
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(false);
            draw(x_, y_, pitch_, true);
            return;
        }
    }

    bool spoilEdge = spoil > 0.5f && !spoilWas_;
    spoilWas_ = spoil > 0.5f;
    fly(nose, spoil);
    if (spoilEdge && mode_ == Mode::Fly) blip(false);

    if (mode_ == Mode::Fly) {
        float windVol = std::clamp((v_ - 8.f) / 36.f, 0.f, 1.f) * 0.055f;
        sys.apu.noise(windVol, 1400.f + v_ * 55.f, false);
        if (vy_ > 0.7f) {
            varioT_ -= DT;
            if (varioT_ <= 0.f) {
                sys.apu.tone(1, 480.f + std::min(vy_, 8.f) * 55.f, 0.045f);
                beep_ = 0.05f;
                varioT_ = std::max(0.08f, 0.28f - vy_ * 0.02f);
            }
        }
        if (v_ < 14.f) sys.apu.tone(2, 160.f, 0.04f);
        else sys.apu.tone(2, 0.f, 0.f);
    }
    draw(x_, y_, pitch_, true);
}

}  // namespace glider

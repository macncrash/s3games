#include "game/kilo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace subkilo {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr float PPM = 5.0f;
constexpr float SUB_X = 78.0f;
constexpr float SUB_R = 12.0f;
constexpr float GATE_M = 1000.0f;
constexpr float GATE_Y = 118.0f;
constexpr float SLOT = 28.0f;
constexpr float AIR0 = 80.0f;

struct WheelDef {
    float m, y, r;
};

constexpr WheelDef kWheels[] = {
    {90, 46, 30},  {190, 186, 34}, {300, 50, 32}, {400, 180, 36}, {510, 42, 28},
    {600, 188, 30}, {700, 58, 36}, {800, 176, 34}, {900, 48, 28},
};

struct Wp {
    float m, y;
};

constexpr Wp kPath[] = {
    {0, 120},  {80, 130},  {160, 120}, {250, 115}, {360, 115}, {470, 120},
    {650, 125}, {760, 120}, {860, 112}, {960, 118}, {1000, 118},
};

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch humPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.4f;
    p.op[0] = {0.5f, 0.8f, 0.05f, 0.4f, 1.0f, 0.4f};
    p.op[1] = {1.0f, 1.0f, 0.08f, 0.5f, 0.8f, 0.35f};
    p.op[2] = {2.0f, 0.3f, 0.05f, 0.4f, 0.5f, 0.3f};
    p.op[3] = {0.5f, 0.2f, 0.1f, 0.5f, 0.4f, 0.3f};
    p.vol = 0.12f;
    p.tone = 700;
    return p;
}

float pathY(float m) {
    if (m <= kPath[0].m) return kPath[0].y;
    for (size_t i = 1; i < sizeof(kPath) / sizeof(kPath[0]); i++) {
        if (m <= kPath[i].m) {
            float u = (m - kPath[i - 1].m) / (kPath[i].m - kPath[i - 1].m);
            return kPath[i - 1].y + (kPath[i].y - kPath[i - 1].y) * u;
        }
    }
    return GATE_Y;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Win || mode_ == Mode::Fail) return 4;
    if (dist_ > 880) return 3;
    if (nearWheel_ > 0) return 2;
    return 1;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.vdp.setFogColor(gs::rgb4(1, 3, 6));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.18f, 0.25f, 0.12f);
    if (bot_) beginLeg();
    else mode_ = Mode::Title;
}

void Game::beginLeg() {
    mode_ = Mode::Dive;
    over_ = false;
    won_ = false;
    touched_ = false;
    missed_ = false;
    dist_ = 0;
    subY_ = 120;
    speed_ = 22;
    air_ = AIR0;
    t_ = 0;
    nearWheel_ = 0;
    banner_.clear();
    sys_->apu.setPatch(0, humPatch());
    sys_->apu.keyOn(0, 55, 0.1f);
    hum_ = 1;
}

float Game::screenX(float meters) const { return SUB_X + (meters - dist_) * PPM; }

bool Game::wheelHit(const Wheel& w, float& sx, float& sy) const {
    sx = screenX(w.m);
    sy = w.y;
    float dx = SUB_X - sx;
    float dy = subY_ - sy;
    return dx * dx + dy * dy < (w.r + SUB_R) * (w.r + SUB_R);
}

void Game::steerBot() {
    float aim = pathY(dist_ + 18);
    float dy = aim - subY_;
    subY_ += std::clamp(dy, -2.4f, 2.4f);
    speed_ = 24;
}

void Game::update(float dt) {
    t_ += dt;
    if (mode_ != Mode::Dive) return;
    gs::Pad& pad = sys_->pad;
    if (bot_) steerBot();
    else {
        float axis = pad.axisY;
        if (pad.down(gs::BTN_UP)) axis += 1;
        if (pad.down(gs::BTN_DOWN)) axis -= 1;
        axis = std::clamp(axis, -1.0f, 1.0f);
        subY_ -= axis * 70.0f * dt;
        speed_ = 20;
        if (pad.down(gs::BTN_RIGHT) || pad.accel > 0.2f) speed_ = 28;
        if (pad.down(gs::BTN_LEFT) || pad.brake > 0.2f) speed_ = 15;
    }
    subY_ = std::clamp(subY_, 40.0f, 190.0f);
    dist_ += speed_ * dt;
    air_ -= dt;
    sys_->apu.setFreq(0, 48 + speed_ * 0.6f);

    nearWheel_ = 0;
    for (const WheelDef& d : kWheels) {
        Wheel w{d.m, d.y, d.r};
        float sx, sy;
        if (std::fabs(d.m - dist_) < 40 && wheelHit(w, sx, sy)) {
            touched_ = true;
            mode_ = Mode::Fail;
            banner_ = "WHEEL";
            over_ = true;
            won_ = false;
            sys_->apu.keyOff(0);
            sys_->apu.noiseBurst(0.4f, 1800, 0.25f);
            return;
        }
        if (std::fabs(d.m - dist_) < 28) nearWheel_ = 1;
    }

    if (dist_ >= GATE_M) {
        float fit = SLOT - SUB_R;
        if (std::fabs(subY_ - GATE_Y) <= fit) {
            mode_ = Mode::Win;
            won_ = true;
            banner_ = "KILO";
            sys_->apu.keyOn(1, 523, 0.2f);
        } else {
            mode_ = Mode::Fail;
            missed_ = true;
            banner_ = "MISSED THE END";
            won_ = false;
            sys_->apu.keyOff(0);
            sys_->apu.noiseBurst(0.25f, 400, 0.4f);
        }
        over_ = true;
        dist_ = GATE_M;
        return;
    }
    if (air_ <= 0) {
        air_ = 0;
        mode_ = Mode::Fail;
        missed_ = true;
        banner_ = "MISSED THE END";
        over_ = true;
        won_ = false;
        sys_->apu.keyOff(0);
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 32 || uc >= 128) {
            width += 4 * scale;
            continue;
        }
        width += (art_.glyph[uc - 32].w * scale + scale);
    }
    if (align == 1) x -= width * 0.5f;
    if (align == 2) x -= width;
    for (char c : s) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 32 || uc >= 128) {
            x += 4 * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[uc - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    const uint16_t sky = gs::rgb4(1, 2, 4);
    const uint16_t deep = gs::rgb4(1, 4, 8);
    const uint16_t floor = gs::rgb4(1, 3, 3);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        float u = y / float(gs::SCREEN_H - 1);
        v.lineBackdrop[y] = y < 18 ? sky : lerpC(deep, floor, (u - 0.08f) / 0.92f);
        v.lineFog[y] = 0;
    }

    if (mode_ == Mode::Title) {
        text("S3 SUB KILO", 160, 70, 2, PAL_HUD, 1);
        text("FINISH THE KILOMETER", 160, 100, 1, PAL_HUD, 1);
        text("DO NOT TOUCH THE WHEELS", 160, 116, 1, PAL_SUB, 1);
        text("MISSING THE END FAILS THE LEG", 160, 132, 1, PAL_HUD, 1);
        text("UP DOWN DEPTH   START DIVE", 160, 168, 1, PAL_HUD, 1);
        text(S3_VERSION_STRING, 160, 200, 1, PAL_HUD, 1);
        return;
    }

    for (int i = 0; i < 8; i++) {
        float m = std::fmod(i * 37.0f - dist_ * 0.35f, 220.0f);
        if (m < 0) m += 220;
        spr(art_.kelp, screenX(dist_ + m * 0.15f), 200, 36, PAL_KELP);
    }
    for (int i = 0; i < 10; i++) {
        float ph = std::fmod(t_ * 18 + i * 41.0f, 240.0f);
        float bx = std::fmod(i * 53.0f + dist_ * 3.0f, 340.0f) - 10;
        spr(art_.bubble, bx, 210 - ph, 8 + (i % 3) * 2, PAL_FX);
    }

    int frame = int(t_ * 8) & 3;
    for (const WheelDef& d : kWheels) {
        float sx = screenX(d.m);
        if (sx < -80 || sx > gs::SCREEN_W + 80) continue;
        int fr = (frame + int(d.m)) & 3;
        spr(art_.wheel[fr], sx, d.y, d.r * 2, PAL_WHEEL);
    }

    float gx = screenX(GATE_M);
    if (gx > -30 && gx < gs::SCREEN_W + 40) {
        float top = GATE_Y - SLOT;
        float bot = GATE_Y + SLOT;
        spr(art_.post, gx, top * 0.45f, top, PAL_GATE);
        spr(art_.post, gx, (bot + gs::SCREEN_H) * 0.5f, gs::SCREEN_H - bot, PAL_GATE);
    }

    spr(art_.sub, SUB_X, subY_, 28, PAL_SUB, false);

    int meters = int(std::min(dist_, GATE_M));
    char buf[64];
    std::snprintf(buf, sizeof(buf), "%04d M", meters);
    text(buf, 8, 12, 1, PAL_HUD, 0);
    std::snprintf(buf, sizeof(buf), "AIR %02d", int(std::ceil(air_)));
    text(buf, 312, 12, 1, PAL_HUD, 2);
    text("LEG 1 KM", 160, 12, 1, PAL_HUD, 1);
    if (nearWheel_ && mode_ == Mode::Dive) text("WHEEL", 160, 28, 1, PAL_WHEEL, 1);
    if (mode_ == Mode::Win) {
        text("KILOMETER", 160, 78, 2, PAL_HUD, 1);
        text("WHEELS CLEAR", 160, 100, 1, PAL_SUB, 1);
        text("END HELD", 160, 116, 1, PAL_GATE, 1);
    } else if (mode_ == Mode::Fail) {
        text(touched_ ? "TOUCHED A WHEEL" : "MISSED THE END", 160, 86, 1, PAL_HUD, 1);
        text("LEG FAILED", 160, 106, 1, PAL_WHEEL, 1);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A) || bot_) beginLeg();
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        if (!bot_ && (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A))) beginLeg();
    } else {
        update(DT);
    }
    draw();
}

}  // namespace subkilo

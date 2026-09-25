#include "game/rickshaw.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace rickshaw {
namespace {

constexpr float kPi = 3.14159265f;
constexpr float kDt = 1.f / 60.f;
constexpr float kKappa = 0.068f;
constexpr float kHalf = 12.5f;
constexpr float kSteer = kKappa * 0.5f;
constexpr float kLeanStick = 0.58f;
constexpr float kCent = 0.08f;
constexpr float kSpring = 28.f;
constexpr float kDamp = 9.6f;
constexpr float kTip = 1.f;
constexpr float kGutter = 4.15f;
constexpr float kRoadHalf = 4.55f;
constexpr float kVSafe = 6.7f;
constexpr float kVCruise = 10.3f;
constexpr float kVCap = 11.2f;
constexpr float kGap = 56.f;
constexpr float kAt0 = 42.f;
constexpr float kStand0 = 196.5f;
constexpr float kStand1 = 214.5f;
constexpr float kMesh = 360.f;
constexpr float kDs = 0.25f;
constexpr float kHorizon = 76.f;
constexpr float kCamH = 1.42f;
constexpr float kFocal = 226.f;
constexpr float kCabH = 106.f;
constexpr float kCabY = 206.f;

struct Bend {
    float at;
    int dir;
    const char* name;
    const char* way;
};

const Bend kBends[3] = {
    {kAt0, -1, "SPICE LANE", "LEAN LEFT"},
    {kAt0 + kGap, 1, "TEMPLE GATE", "LEAN RIGHT"},
    {kAt0 + 2.f * kGap, -1, "STATION BEND", "LEAN LEFT"},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

float Game::bendAt(float s) const {
    float b = 0.f;
    for (const Bend& d : kBends) {
        float z = (s - d.at) / kHalf;
        if (z <= -1.f || z >= 1.f) continue;
        b += float(d.dir) * 0.5f * (1.f + std::cos(kPi * z));
    }
    return b;
}

void Game::centerAt(float s, float& x, float& y, float& h) const {
    if (road_.empty()) {
        x = y = h = 0.f;
        return;
    }
    if (s <= 0.f) {
        x = road_.front().x;
        y = road_.front().y;
        h = road_.front().h;
        return;
    }
    const Node& last = road_.back();
    if (s >= last.s) {
        float d = s - last.s;
        h = last.h;
        x = last.x + std::sin(h) * d;
        y = last.y + std::cos(h) * d;
        return;
    }
    int i = int(s / kDs);
    if (i < 0) i = 0;
    if (i >= int(road_.size()) - 1) i = int(road_.size()) - 2;
    const Node& a = road_[size_t(i)];
    const Node& b = road_[size_t(i + 1)];
    float span = b.s - a.s;
    float t = span > 1e-4f ? (s - a.s) / span : 0.f;
    t = clampf(t, 0.f, 1.f);
    x = a.x + (b.x - a.x) * t;
    y = a.y + (b.y - a.y) * t;
    h = a.h + (b.h - a.h) * t;
}

void Game::buildMesh() {
    road_.clear();
    float x = 0, y = 0, h = 0, s = 0;
    road_.push_back({s, x, y, h});
    while (s < kMesh) {
        float k = bendAt(s) * kKappa;
        h += k * kDs;
        x += std::sin(h) * kDs;
        y += std::cos(h) * kDs;
        s += kDs;
        road_.push_back({s, x, y, h});
    }
}

void Game::layout() {
    props_.clear();
    auto add = [&](float s, float lat, int kind) {
        Prop p;
        p.s = s;
        p.lat = lat;
        p.kind = kind;
        float x, y, h;
        centerAt(s, x, y, h);
        float rx = std::cos(h), ry = -std::sin(h);
        p.wx = x + rx * lat;
        p.wy = y + ry * lat;
        props_.push_back(p);
    };
    for (float s = 8.f; s < 250.f; s += 9.f) {
        if (s > kStand0 - 8.f && s < kStand0 + 10.f) continue;
        int n = int(s);
        add(s, -8.8f, n % 4);
        add(s + 4.5f, 9.f, (n + 1) % 4);
    }
    for (float s = 14.f; s < 250.f; s += 16.f) {
        add(s, -5.9f, Lamp);
        add(s + 8.f, 6.f, Lamp);
    }
    for (float s = 20.f; s < 240.f; s += 28.f) add(s, 7.2f, Stall);
    for (const Bend& d : kBends) add(d.at - kHalf - 1.5f, d.dir < 0 ? -6.4f : 6.4f, d.dir < 0 ? SignL : SignR);
    add(kStand0, -5.2f, Post);
    add(kStand0, 5.2f, Post);
    add(kStand0 + 3.f, -3.4f, Flower);
    add(kStand0 + 3.f, 3.4f, Flower);
    add(kStand0 + 6.f, -3.1f, Flower);
    add(kStand0 + 6.f, 3.1f, Flower);
    add(24.f, -9.6f, Cow);
    add(78.f, 9.4f, Dog);
}

void Game::begin() {
    s_ = x_ = psi_ = v_ = lean_ = leanV_ = 0.f;
    job_ = settle_ = shake_ = 0.f;
    over_ = won_ = spilled_ = false;
    fail_ = "";
    chime_ = chimeStep_ = 0;
    chimeT_ = bellT_ = toneT_ = 0.f;
}

void Game::tip(const char* why) {
    spilled_ = true;
    won_ = false;
    over_ = true;
    mode_ = Mode::Fail;
    fail_ = why;
    shake_ = 1.f;
    sys_->apu.noiseBurst(0.45f, 180.f, 0.35f);
    blip(110.f);
}

void Game::deliver() {
    won_ = true;
    over_ = true;
    spilled_ = false;
    mode_ = Mode::Win;
    fail_ = "";
    chime_ = 5;
    chimeStep_ = 0;
    chimeT_ = 0.f;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    toneT_ = 0.14f;
}

void Game::pilot(float& u, bool& pedal, bool& brake) const {
    float b = bendAt(s_);
    float ahead = bendAt(s_ + 14.f);
    float tv = kVCruise;
    if (std::fabs(b) > 0.02f || std::fabs(ahead) > 0.02f) tv = kVSafe;
    if (s_ > kStand0 - 20.f) {
        float d = (kStand0 + 4.f) - s_;
        tv = std::min(tv, std::max(0.32f, d * 0.33f));
    }
    pedal = v_ < tv - 0.2f;
    brake = v_ > tv + 0.15f;
    if (std::fabs(b) > 0.16f) {
        u = b > 0.f ? 1.f : -1.f;
    } else {
        float psiT = clampf(-0.5f * x_, -0.42f, 0.42f);
        u = clampf(2.8f * (psiT - psi_), -1.f, 1.f);
    }
}

void Game::physics(float u, bool pedal, bool brake) {
    float drive = 0.f;
    if (pedal && !brake) drive += 14.f;
    if (brake) drive -= 18.f;
    drive -= v_ * 0.7f + v_ * v_ * 0.018f;
    v_ = clampf(v_ + drive * kDt, 0.f, kVCap);

    float b = bendAt(s_);
    float k = b * kKappa;
    psi_ += (u * kSteer - k) * v_ * kDt;
    s_ += v_ * std::cos(psi_) * kDt;
    x_ += v_ * std::sin(psi_) * kDt;

    float cent = v_ * v_ * k * kCent;
    float target = u * kLeanStick + cent;
    leanV_ += (kSpring * (target - lean_) - kDamp * leanV_) * kDt;
    lean_ += leanV_ * kDt;

    if (std::fabs(psi_) > 1.2f || std::fabs(x_) > kGutter || std::fabs(lean_) > kTip) {
        tip("THE CAB TIPPED");
        return;
    }
    int cleared = 0;
    for (const Bend& d : kBends)
        if (s_ > d.at) cleared++;
    bool bay = s_ >= kStand0 && s_ <= kStand1 && cleared >= 3 && v_ < 1.7f && std::fabs(x_) < 1.05f &&
               std::fabs(psi_) < 0.5f && std::fabs(lean_) < 0.5f;
    if (bay) {
        settle_ += kDt;
        if (settle_ >= 0.35f) deliver();
    } else {
        settle_ = 0.f;
    }
    if (!won_ && s_ > kStand1) tip("MISSED THE STAND");
}

const char* Game::hint() const {
    if (mode_ == Mode::Win) return "THE FARE IS PAID";
    if (mode_ == Mode::Fail) return fail_;
    float b = bendAt(s_);
    float ahead = bendAt(s_ + 14.f);
    if (s_ >= kStand0 - 10.f && s_ <= kStand1) {
        if (v_ > 1.7f) return "EASE OFF AT THE STAND";
        if (std::fabs(lean_) > 0.4f) return "LEVEL THE CAB";
        if (std::fabs(x_) > 0.9f) return "CENTER THE CAB";
        return "HOLD STILL FOR THE FARE";
    }
    if ((std::fabs(ahead) > 0.12f || std::fabs(b) > 0.08f) && v_ > 8.4f) return "BRAKE FOR THE BEND";
    if (b > 0.16f) return "LEAN RIGHT";
    if (b < -0.16f) return "LEAN LEFT";
    if (std::fabs(x_) > 1.3f) return "CENTER THE CAB";
    if (v_ < 0.8f) return "C PEDALS THE LANE";
    return "STRAIGHT  KEEP THE CAB LEVEL";
}

void Game::audio(float dt) {
    gs::APU& apu = sys_->apu;
    if (mode_ == Mode::Play) {
        apu.noise(0.012f + v_ * 0.0035f, 220.f + v_ * 64.f, false);
        apu.tone(2, 46.f + v_ * 5.2f, 0.018f + v_ * 0.0018f);
    } else if (mode_ == Mode::Title) {
        apu.noise(0.008f, 160.f, false);
        apu.tone(2, 0.f, 0.f);
    } else {
        apu.noise(0.005f, 90.f, false);
        apu.tone(2, 0.f, 0.f);
    }
    if (toneT_ > 0.f) {
        toneT_ -= dt;
        if (toneT_ <= 0.f && chime_ == 0) apu.tone(0, 0.f, 0.f);
    }
    if (bellT_ > 0.f) {
        bellT_ -= dt;
        if (bellT_ <= 0.f) apu.tone(1, 0.f, 0.f);
    }
    if (chime_ > 0) {
        chimeT_ -= dt;
        if (chimeT_ <= 0.f) {
            static const float notes[] = {523.25f, 659.25f, 783.99f, 1046.5f, 783.99f};
            int n = std::min(chimeStep_, 4);
            apu.tone(0, notes[n], 0.05f);
            toneT_ = 0.16f;
            chimeT_ = 0.15f;
            if (++chimeStep_ >= chime_) chime_ = 0;
        }
    }
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - dt);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, int fog, bool flip, bool shadow) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (cx + w * 0.5f < -40.f || cy + h * 0.5f < -40.f || cx - w * 0.5f > gs::SCREEN_W + 40.f ||
        cy - h * 0.5f > gs::SCREEN_H + 40.f)
        return;
    gs::Sprite s;
    long sw = std::clamp(std::lround(w), 1L, 900L);
    long sh = std::clamp(std::lround(h), 1L, 900L);
    s.w = int16_t(sw);
    s.h = int16_t(sh);
    s.x = int16_t(std::clamp(std::lround(cx - sw * 0.5f), -2000L, 2000L));
    s.y = int16_t(std::clamp(std::lround(cy - sh * 0.5f), -2000L, 2000L));
    s.img = m.pick(float(sh));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* text, int pal) {
    if (!text || row < 0 || row > 27) return;
    for (int i = 0; text[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(text[i]);
        if (x < 0 || x > 39 || c < 32 || c > 127 || c == ' ') continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* text, int pal) {
    int n = 0;
    if (text)
        while (text[n]) n++;
    hud(20 - n / 2, row, text, pal);
}

void Game::drawSky(float s) {
    gs::VDP& v = sys_->vdp;
    (void)s;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(kHorizon)) {
            float t = float(y) / kHorizon;
            int r = int(4.f + t * 10.f);
            int g = int(5.f + t * 4.f);
            int b = int(11.f - t * 6.f);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
        } else {
            v.lineBackdrop[y] = gs::rgb4(6, 5, 3);
            float fog = clampf((kHorizon + 28.f - float(y)) / 28.f, 0.f, 1.f);
            v.lineFog[y] = uint8_t(fog * 7.f);
        }
    }
}

void Game::drawRoad(float s, float lat, float psi) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) v.road[y].on = false;

    float H = -psi;
    float X = -lat;
    float Z = 0.f;
    float u = 0.f;
    const float du = 0.28f;
    bool have = false;
    int lastY = 0;
    float lastCx = 0, lastHw = 0, lastU = 0;
    for (int n = 0; n < 460; n++) {
        float k = bendAt(s + u) * kKappa;
        float mid = H + k * du * 0.5f;
        X += std::sin(mid) * du;
        Z += std::cos(mid) * du;
        H += k * du;
        u += du;
        if (Z < 0.8f) continue;
        int y = int(std::lround(kHorizon + kCamH * kFocal / Z));
        float cx = 160.f + X * kFocal / Z;
        float hw = std::max(0.6f, kRoadHalf * kFocal / Z);
        if (have && y != lastY) {
            int y0 = std::min(y, lastY);
            int y1 = std::max(y, lastY);
            for (int yy = y0; yy <= y1; yy++) {
                if (yy <= int(kHorizon) || yy >= gs::SCREEN_H || v.road[yy].on) continue;
                float den = float(y - lastY);
                float t = den != 0.f ? float(yy - lastY) / den : 0.f;
                gs::RoadLine& rl = v.road[yy];
                rl.on = true;
                rl.cx = lastCx + (cx - lastCx) * t;
                rl.hw = std::max(0.6f, lastHw + (hw - lastHw) * t);
                float uu = lastU + (u - lastU) * t;
                rl.v = (s + uu) * 150.f;
                rl.pal = PAL_STREET;
                rl.style = 1;
                rl.band = (s + uu >= kStand0 && s + uu <= kStand1) ? 1 : 0;
                rl.left = rl.right = gs::GROUND_LAND;
            }
        }
        have = true;
        lastY = y;
        lastCx = cx;
        lastHw = hw;
        lastU = u;
        if (Z > 96.f) break;
    }
    int nearest = -1;
    for (int y = gs::SCREEN_H - 1; y > int(kHorizon); --y) {
        if (v.road[y].on) {
            nearest = y;
            break;
        }
    }
    if (nearest > 0) {
        for (int y = nearest + 1; y < gs::SCREEN_H; y++) {
            v.road[y] = v.road[nearest];
            v.road[y].hw = v.road[nearest].hw + float(y - nearest) * 4.f;
        }
    }
    for (int pass = 0; pass < 3; pass++) {
        for (int y = int(kHorizon) + 2; y < gs::SCREEN_H - 1; y++) {
            if (!v.road[y].on && v.road[y - 1].on && v.road[y + 1].on) v.road[y] = v.road[y + 1];
        }
    }
}

void Game::drawProps(float s, float lat, float psi) {
    float lx, ly, hdg;
    centerAt(s, lx, ly, hdg);
    float roadRx = std::cos(hdg), roadRy = -std::sin(hdg);
    float camX = lx + roadRx * lat;
    float camY = ly + roadRy * lat;
    float ch = hdg + psi;
    float fwdX = std::sin(ch), fwdY = std::cos(ch);
    float rightX = std::cos(ch), rightY = -std::sin(ch);

    struct Item {
        float z;
        int index;
    };
    std::vector<Item> order;
    order.reserve(props_.size());
    for (int i = 0; i < int(props_.size()); i++) {
        float dx = props_[size_t(i)].wx - camX;
        float dy = props_[size_t(i)].wy - camY;
        float z = dx * fwdX + dy * fwdY;
        if (z < 2.4f || z > 54.f) continue;
        order.push_back({z, i});
    }
    std::sort(order.begin(), order.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : order) {
        const Prop& p = props_[size_t(it.index)];
        float dx = p.wx - camX;
        float dy = p.wy - camY;
        float z = it.z;
        float xc = dx * rightX + dy * rightY;
        float ground = kHorizon + kCamH * kFocal / z;
        float sx = 160.f + xc * kFocal / z;
        const gs::Mipped* m = &art_.shop;
        int pal = PAL_CITY;
        float worldH = 5.2f;
        bool flip = false;
        switch (p.kind) {
            case Shop:
                m = &art_.shop;
                worldH = 5.0f;
                flip = p.lat < 0.f;
                break;
            case House:
                m = &art_.house;
                worldH = 6.4f;
                flip = p.lat > 0.f;
                break;
            case Temple:
                m = &art_.temple;
                pal = PAL_TEMPLE;
                worldH = 7.0f;
                break;
            case Shed:
                m = &art_.shed;
                worldH = 4.2f;
                flip = p.lat < 0.f;
                break;
            case Lamp:
                m = &art_.lamp;
                worldH = 3.6f;
                break;
            case Stall:
                m = &art_.stall;
                worldH = 2.3f;
                break;
            case SignL:
                m = &art_.signL;
                pal = PAL_SIGN;
                worldH = 3.3f;
                break;
            case SignR:
                m = &art_.signR;
                pal = PAL_SIGN;
                worldH = 3.3f;
                break;
            case Post:
                m = &art_.post;
                pal = PAL_SIGN;
                worldH = 4.6f;
                break;
            case Flower:
                m = &art_.flower;
                pal = PAL_LIFE;
                worldH = 1.1f;
                break;
            case Cow:
                m = &art_.cow;
                pal = PAL_LIFE;
                worldH = 1.45f;
                break;
            case Dog:
                m = &art_.dog;
                pal = PAL_LIFE;
                worldH = 0.85f;
                break;
            default:
                break;
        }
        float sh = worldH * kFocal / z;
        if (sh < 2.f || sh > 380.f) continue;
        float sy = ground - sh * 0.5f;
        int fog = int(clampf((z - 7.f) * 0.38f, 0.f, 13.f));
        spr(*m, sx, sy, sh, pal, fog, flip, false);
    }
}

void Game::drawCab(float lean, bool spilled) {
    float bob = std::sin(t_ * 9.f) * (mode_ == Mode::Play ? std::min(v_ * 0.12f, 1.4f) : 0.4f);
    float shake = std::sin(t_ * 47.f) * shake_ * 3.f;
    float px = 160.f + shake;
    float py = kCabY + bob;
    if (spilled) {
        spr(art_.spilled, px - 10.f, py - 40.f, 96.f, PAL_CAB, 0, false, false);
        return;
    }
    float u = clampf(lean / 0.92f, -1.f, 1.f);
    int i = int(std::lround((u * 0.5f + 0.5f) * 8.f));
    i = std::clamp(i, 0, 8);
    const gs::Mipped& m = art_.cab[i];
    float sc = kCabH / float(m.h);
    float dw = float(m.w) * sc;
    float left = px - art_.pivX * sc;
    float top = py - art_.pivY * sc;
    spr(m, left + dw * 0.5f, top + kCabH * 0.5f, kCabH, PAL_CAB, 0, false, false);
    spr(art_.dust, px, py + 6.f, 18.f, PAL_LIFE, 0, false, true);
}

void Game::drawHud() {
    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(21, "ONE FARE. THREE TURNS.", PAL_BANNER);
        hudC(22, "LEAN INTO EACH BEND", PAL_HUD);
        hudC(23, "ARROWS LEAN    C PEDAL    X BRAKE", PAL_HUD);
        hudC(24, "DON'T TIP THE CAB", PAL_ALERT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "START", PAL_WIN);
        return;
    }
    hud(1, 0, "S3 RICKSHAW", PAL_BANNER);
    std::snprintf(buf, sizeof buf, "SPD %4.1f", v_);
    hud(30, 0, buf, v_ > 8.6f ? PAL_ALERT : PAL_HUD);

    const Bend* next = nullptr;
    int passed = 0;
    for (const Bend& d : kBends) {
        if (s_ > d.at) passed++;
        if (!next && s_ < d.at + kHalf) next = &d;
    }
    if (passed >= 3) {
        hud(1, 1, "THE STAND", PAL_WIN);
    } else if (next) {
        std::snprintf(buf, sizeof buf, "TURN %d OF 3  %s", passed + 1, next->name);
        hud(1, 1, buf, PAL_BANNER);
        if (std::fabs(bendAt(s_)) > 0.12f) hud(1, 2, next->way, PAL_ALERT);
    }

    char cell[12];
    std::snprintf(cell, sizeof cell, "-----|-----");
    int mark = int(std::lround(clampf(lean_, -1.f, 1.f) * 5.f + 5.f));
    mark = std::clamp(mark, 0, 10);
    cell[mark] = 'O';
    std::snprintf(buf, sizeof buf, "LEAN %s", cell);
    hud(1, 3, buf, std::fabs(lean_) > 0.82f ? PAL_ALERT : PAL_HUD);

    if (mode_ == Mode::Pause) {
        hudC(12, "PAUSED", PAL_BANNER);
        hudC(14, "START CONTINUES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(16, "NINA IS AT THE STAND", PAL_WIN);
        hudC(17, "THREE TURNS  CAB UP", PAL_HUD);
        if (!bot_) hudC(19, "START FOR ANOTHER FARE", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fail) {
        hudC(16, fail_, PAL_ALERT);
        if (!bot_) hudC(18, "START TRIES AGAIN", PAL_HUD);
        return;
    }
    const char* h = hint();
    int pal = PAL_HUD;
    if (h && (std::strstr(h, "BRAKE") || std::strstr(h, "LEAN"))) pal = PAL_ALERT;
    if (h && std::strstr(h, "HOLD")) pal = PAL_WIN;
    hudC(26, h, pal);
    hud(1, 27, "FARE NINA", PAL_BANNER);
    hud(28, 27, "NO TIP", PAL_ALERT);
}

void Game::draw(float s, float lat, float psi, float lean) {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    drawSky(s);
    drawRoad(s, lat, psi);

    if (mode_ == Mode::Title) {
        spr(art_.title, 160.f, 28.f, float(art_.title.h), PAL_BANNER, 0, false, false);
        spr(art_.sub, 160.f, 58.f, float(art_.sub.h), PAL_HUD, 0, false, false);
        int flap = int(t_ * 3.f) & 1;
        for (int i = 0; i < 7; i++) {
            float x = 40.f + float(i) * 40.f;
            float y = 78.f + std::sin(t_ * 2.f + float(i)) * 2.f;
            spr(art_.flower, x, y, 14.f, PAL_LIFE, 0, false, false);
            (void)flap;
        }
    } else if (mode_ == Mode::Win) {
        spr(art_.paid, 160.f, 36.f, float(art_.paid.h) * 1.1f, PAL_WIN, 0, false, false);
        for (int i = 0; i < 8; i++) {
            float x = 28.f + float(i) * 36.f;
            float y = 62.f + std::sin(t_ * 3.f + float(i) * 0.7f) * 4.f;
            spr(art_.flower, x, y, 16.f, PAL_LIFE, 0, false, false);
        }
    } else if (mode_ == Mode::Fail) {
        spr(art_.tipped, 160.f, 34.f, float(art_.tipped.h) * 1.15f, PAL_ALERT, 0, false, false);
    }

    drawCab(lean, spilled_ && mode_ == Mode::Fail);
    if (mode_ == Mode::Play && v_ > 2.4f) {
        for (int i = 0; i < 3; i++) {
            float life = std::fmod(t_ * 3.4f + float(i) * 0.37f, 1.f);
            float x = 160.f + (i - 1) * 28.f;
            float y = kCabY - 8.f + life * 10.f;
            spr(art_.dust, x, y, 10.f + life * 8.f, PAL_CITY, int(life * 8.f), false, false);
        }
    }
    drawProps(s, lat, psi);

    int flap = int(t_ * 4.f) & 1;
    spr(art_.bird[flap], 70.f + std::sin(t_ * 0.7f) * 18.f, 40.f, 12.f, PAL_LIFE, 2, false, false);
    spr(art_.bird[1 - flap], 250.f + std::cos(t_ * 0.5f) * 14.f, 32.f, 10.f, PAL_LIFE, 3, true, false);
    spr(art_.sun, 268.f, 58.f, 34.f, PAL_LIFE, 1, false, false);
    drawHud();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildMesh();
    layout();
    begin();
    if (bot_) mode_ = Mode::Play;
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || (bot_ && t_ < kDt * 2.f)) {
            begin();
            mode_ = Mode::Play;
            blip(660.f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(330.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        } else {
            job_ += kDt;
            float u = 0.f;
            bool pedal = false, brake = false;
            if (bot_) pilot(u, pedal, brake);
            else {
                u = pad.axisX;
                if (pad.down(gs::BTN_LEFT)) u -= 1.f;
                if (pad.down(gs::BTN_RIGHT)) u += 1.f;
                u = clampf(u, -1.f, 1.f);
                pedal = pad.down(gs::BTN_C) || pad.down(gs::BTN_UP) || pad.accel > 0.25f;
                brake = pad.down(gs::BTN_X) || pad.down(gs::BTN_B) || pad.down(gs::BTN_DOWN) || pad.brake > 0.25f;
                if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_TURBO)) {
                    sys.apu.tone(1, 988.f, 0.05f);
                    bellT_ = 0.12f;
                }
            }
            physics(u, pedal, brake);
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    } else if (mode_ == Mode::Win || mode_ == Mode::Fail) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            begin();
            mode_ = Mode::Play;
            blip(660.f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            begin();
            mode_ = Mode::Title;
        }
    }

    float camS = s_, camX = x_, camPsi = psi_, camLean = lean_;
    if (mode_ == Mode::Title) {
        camS = 6.f + std::fmod(t_ * 2.6f, 20.f);
        camX = 0.f;
        camPsi = std::sin(t_ * 0.4f) * 0.02f;
        camLean = std::sin(t_ * 1.25f) * 0.22f;
    }
    audio(kDt);
    draw(camS, camX, camPsi, camLean);
}

}  // namespace rickshaw

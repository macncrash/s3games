#include "game/turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace kartturn {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float FINISH = 318.f;
constexpr float LEG = 26.f;
constexpr float HORIZON = 76.f;
constexpr int HORIZON_I = 76;
constexpr float MAX_LEAN = 1.f;
constexpr float LEAN_RATE = 2.4f;
constexpr float LAT_FAIL = 2.15f;

struct Corner {
    float a, b;
    float lean;
    const char* name;
};

const Corner CORNERS[3] = {
    {38.f, 92.f, -0.58f, "LEFT BEND"},
    {124.f, 178.f, 0.74f, "RIGHT HOOK"},
    {214.f, 272.f, -0.66f, "LEFT SWEEP"},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float smooth(float z, float a, float b) {
    if (z <= a || z >= b) return 0.f;
    float u = (z - a) / (b - a);
    return std::sin(u * 3.14159265f);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    won_ = false;
    over_ = false;
    cleared_ = 0;
    t_ = 0;
    std::snprintf(result_, sizeof result_, "S3 KARTTURN  FAIL  missed the end");
    if (bot_) resetRide();
    else mode_ = Mode::Title;
    sys.apu.setMaster(0.75f);
}

void Game::resetRide() {
    mode_ = Mode::Ride;
    modeT_ = 0;
    clock_ = 0;
    z_ = 0;
    lean_ = 0;
    lat_ = 0;
    tip_ = 0;
    speed_ = 14.f;
    shake_ = 0;
    cleared_ = 0;
    passed_[0] = passed_[1] = passed_[2] = false;
    won_ = false;
    over_ = false;
}

void Game::finish(bool win, const char* why) {
    won_ = win;
    mode_ = win ? Mode::Win : Mode::Fail;
    modeT_ = 0;
    shake_ = win ? 0 : 4;
    if (win) {
        std::snprintf(result_, sizeof result_, "S3 KARTTURN  PASS  three turns without tipping  %d/3", cleared_);
        sys_->apu.tone(0, 523.f, 0.25f);
        sys_->apu.tone(1, 659.f, 0.2f);
        sys_->apu.tone(2, 784.f, 0.18f);
    } else {
        std::snprintf(result_, sizeof result_, "S3 KARTTURN  FAIL  %s  %d/3", why, cleared_);
        sys_->apu.tone(0, 84.f, 0.35f);
        sys_->apu.tone(1, 54.f, 0.28f);
        sys_->apu.tone(2, 0.f, 0.f);
    }
}

float Game::needAt(float z) const {
    float n = 0.f;
    for (const Corner& c : CORNERS) {
        float w = smooth(z, c.a - 6.f, c.b + 4.f);
        if (w > 0.f) n += c.lean * w;
    }
    return clampf(n, -MAX_LEAN, MAX_LEAN);
}

float Game::bendAt(float z) const {
    float b = 0.f;
    for (const Corner& c : CORNERS) b += c.lean * smooth(z, c.a - 2.f, c.b + 12.f);
    return b;
}

int Game::cornerAt(float z) const {
    for (int i = 0; i < 3; i++)
        if (z >= CORNERS[i].a && z < CORNERS[i].b) return i;
    return -1;
}

float Game::steerIn() const {
    if (bot_) {
        float look = needAt(z_ + 14.f);
        return clampf((look - lean_) * 3.6f, -1.f, 1.f);
    }
    const gs::Pad& p = sys_->pad;
    float a = p.axisX;
    if (p.down(gs::BTN_LEFT)) a -= 1.f;
    if (p.down(gs::BTN_RIGHT)) a += 1.f;
    return clampf(a, -1.f, 1.f);
}

float Game::gasIn() const {
    if (bot_) return 0.62f;
    const gs::Pad& p = sys_->pad;
    float g = p.accel;
    if (p.down(gs::BTN_A) || p.down(gs::BTN_C)) g = 1.f;
    if (p.down(gs::BTN_B) || p.brake > 0.2f) g = 0.f;
    return g;
}

void Game::ride(float dt) {
    float gas = gasIn();
    bool brake = !bot_ && (sys_->pad.down(gs::BTN_B) || sys_->pad.brake > 0.2f);
    float cruise = 11.f + gas * 10.f;
    if (brake) cruise = 7.f;
    speed_ += (cruise - speed_) * dt * 1.6f;
    speed_ = clampf(speed_, 6.f, 24.f);

    float grip = clampf(speed_ / 18.f, 0.45f, 1.05f);
    float want = steerIn() * MAX_LEAN * grip;
    float d = want - lean_;
    float step = LEAN_RATE * dt;
    if (std::fabs(d) <= step) lean_ = want;
    else lean_ += (d > 0.f ? step : -step);

    z_ += speed_ * dt;
    clock_ += dt;

    int ci = cornerAt(z_);
    float need = needAt(z_);
    float err = lean_ - need;
    lat_ += err * speed_ * 0.045f * dt;
    lat_ *= (1.f - 0.35f * dt);
    if (std::fabs(lat_) > LAT_FAIL) {
        finish(false, "missed the end");
        return;
    }

    if (ci >= 0) {
        float ae = std::fabs(err);
        if (ae > 0.36f) tip_ += dt * (0.55f + (ae - 0.36f) * 1.6f) * (0.7f + speed_ / 30.f);
        else tip_ -= dt * 1.05f;
    } else {
        tip_ -= dt * 0.8f;
        if (std::fabs(lean_) > 0.9f) tip_ += dt * 0.4f;
    }
    tip_ = clampf(tip_, 0.f, 1.f);
    if (tip_ >= 1.f) {
        finish(false, "tipped");
        return;
    }

    for (int i = 0; i < 3; i++) {
        if (!passed_[i] && z_ >= CORNERS[i].b) {
            passed_[i] = true;
            cleared_++;
            sys_->apu.tone(1, 392.f + float(i) * 90.f, 0.16f);
        }
    }

    if (z_ >= FINISH) {
        if (cleared_ >= 3) finish(true, "clean");
        else finish(false, "missed the end");
        return;
    }
    if (clock_ >= LEG) {
        finish(false, "missed the end");
        return;
    }

    float rpm = 62.f + speed_ * 7.f + std::fabs(lean_) * 14.f;
    sys_->apu.tone(0, rpm, 0.055f);
    sys_->apu.tone(2, rpm * 0.5f, 0.025f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);

    if (mode_ == Mode::Title) {
        lean_ = std::sin(t_ * 1.2f) * 0.42f;
        if (start) resetRide();
    } else if (mode_ == Mode::Ride) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else ride(DT);
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) mode_ = Mode::Ride;
        sys.apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        modeT_ += DT;
        sys.apu.tone(0, mode_ == Mode::Win ? 170.f : 48.f, modeT_ < 0.35f ? 0.12f : 0.f);
        if (bot_ && modeT_ > 1.05f) over_ = true;
        else if (!bot_ && start && modeT_ > 0.4f) {
            if (mode_ == Mode::Win) mode_ = Mode::Title;
            else resetRide();
        }
    }

    draw();
    if (!sys.headless) {
        if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
        else if (mode_ == Mode::Fail) sys.setLight(200, 30, 20);
        else if (tip_ > 0.55f) sys.setLight(200, 120, 20);
        else sys.setLight(40, 90, 180);
    }
}

void Game::blit(const gs::Mipped& m, float x, float y, float h, int pal, bool flip) {
    if (m.h < 1 || h < 2.f) return;
    float s = h / float(m.h);
    float w = float(m.w) * s;
    if (x + w < -8.f || y + h < -8.f || x > gs::SCREEN_W || y > gs::SCREEN_H) return;
    gs::Sprite sp;
    sp.x = int16_t(std::lround(x));
    sp.y = int16_t(std::lround(y));
    sp.w = int16_t(std::lround(w));
    sp.h = int16_t(std::lround(h));
    sp.img = m.pick(h);
    sp.pal = uint8_t(pal);
    sp.hflip = flip;
    sys_->vdp.sprite(sp);
}

void Game::image(const gs::Image& img, float x, float y, int pal) {
    if (!img.w || !img.h) return;
    gs::Sprite sp;
    sp.x = int16_t(std::lround(x));
    sp.y = int16_t(std::lround(y));
    sp.w = int16_t(img.w);
    sp.h = int16_t(img.h);
    sp.img = img;
    sp.pal = uint8_t(pal);
    sys_->vdp.sprite(sp);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    v.A.enabled = false;
    v.B.enabled = false;
    int shake = shake_ ? ((int(t_ * 60.f) & 1) ? shake_ : -shake_) : 0;
    float miss = lean_ - bendAt(z_ + 8.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int yy = y;
        if (y > HORIZON_I) yy = std::max(0, std::min(gs::SCREEN_H - 1, y + shake));
        float u = y < HORIZON_I ? y / HORIZON : 1.f;
        int r = int(3 + 7 * u);
        int g = int(5 + 4 * u);
        int b = int(10 + 2 * (1.f - u));
        if (mode_ == Mode::Fail && modeT_ < 0.12f) r = std::min(15, r + 6);
        v.lineBackdrop[yy] = gs::rgb4(r, g, b);
        v.lineFog[y] = uint8_t(y < HORIZON_I ? (HORIZON_I - y) / 8 : 0);
        gs::RoadLine& road = v.road[yy];
        if (y <= HORIZON_I) {
            road.on = false;
            continue;
        }
        float dd = float(y - HORIZON_I);
        float dz = 480.f / dd;
        float wz = z_ + dz;
        float near = 86.f / (dz + 12.f);
        float cx = 160.f - bendAt(wz) * 84.f * near - (miss + lat_) * 40.f * near;
        road.on = true;
        road.cx = cx;
        road.hw = 12.f + dd * 1.22f;
        road.v = wz * 0.32f;
        road.pal = PAL_FIELD;
        road.band = (int(wz * 0.5f) & 4) ? 1 : 0;
        road.style = 1;
        road.left = 0;
        road.right = 0;
    }
}

void Game::hud() {
    gs::VDP& v = sys_->vdp;
    v.HUD.clear();
    v.hudEnabled = true;
    auto text = [&](int col, int row, const std::string& s, int pal) {
        for (size_t i = 0; i < s.size(); i++) {
            int x = col + int(i);
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
            if (x < 0 || x > 39 || row < 0 || row > 27 || c < 32 || c >= 128) continue;
            int tile = art_.font[c - 32];
            if (!tile) continue;
            v.HUD.set(x, row, gs::entry(tile, pal));
        }
    };

    if (mode_ == Mode::Title) {
        text(7, 20, "LEFT AND RIGHT SET THE LEAN", PAL_HUD);
        text(6, 22, "HOLD A FOR GAS  B TO COAST", PAL_HUD);
        text(6, 24, "THREE TURNS THEN THE BANNER", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) text(11, 26, "START TO ROLL", PAL_HUD);
        return;
    }

    int show = mode_ == Mode::Win ? 3 : cleared_;
    char line[48];
    std::snprintf(line, sizeof line, "TURNS %d/3", show);
    text(1, 0, line, PAL_HUD);
    int left = int(std::ceil(std::max(0.f, LEG - clock_)));
    if (mode_ == Mode::Win) left = 0;
    std::snprintf(line, sizeof line, "LEG %02d", left);
    text(32, 0, line, left <= 5 && mode_ == Mode::Ride ? 4 : PAL_HUD);

    int ci = cornerAt(z_);
    if (mode_ == Mode::Ride && ci >= 0) text(13, 0, CORNERS[ci].name, tip_ > 0.45f ? 4 : 3);
    else if (mode_ == Mode::Ride) text(15, 0, "STRAIGHT", PAL_HUD);

    int slots = 15;
    int mark = int(std::lround((lean_ * 0.5f + 0.5f) * (slots - 1)));
    mark = std::max(0, std::min(slots - 1, mark));
    std::string bar = "LEAN ";
    for (int i = 0; i < slots; i++) bar += (i == mark) ? '|' : (i == slots / 2 ? '+' : '-');
    text(1, 26, bar, tip_ > 0.45f ? 4 : PAL_HUD);
    int tipn = int(std::lround(tip_ * 8.f));
    std::string tb = "TIP ";
    for (int i = 0; i < 8; i++) tb += (i < tipn) ? '#' : '.';
    text(28, 26, tb, tipn >= 5 ? 4 : PAL_HUD);

    if (mode_ == Mode::Pause) text(16, 12, "PAUSED", 4);
    if (mode_ == Mode::Fail) {
        text(8, 11, won_ ? "CLEAN" : (tip_ >= 1.f ? "TIPPED THE KART" : "MISSED THE END"), 4);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(12, 14, "START RETRIES", PAL_HUD);
    }
    if (mode_ == Mode::Win) {
        text(5, 11, "THREE TURNS  LEG DONE", 5);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(13, 14, "START AGAIN", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    backdrop();
    hud();

    if (mode_ == Mode::Title || mode_ == Mode::Win) {
        image(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 26.f, PAL_TITLE);
        image(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 64.f, PAL_TITLE);
    }

    float miss = lean_ - bendAt(z_ + 8.f);
    for (int i = 0; i < 3; i++) {
        float dz = CORNERS[i].a - z_;
        if (dz < 3.f || dz > 72.f) continue;
        float near = 86.f / (dz + 12.f);
        float x = 160.f - bendAt(z_ + dz) * 84.f * near - (miss + lat_) * 22.f * near;
        float y = HORIZON + (gs::SCREEN_H - HORIZON - 18.f) * (6.f / (dz + 6.f));
        float h = 16.f + 36.f * near;
        const gs::Mipped& ch = CORNERS[i].lean < 0.f ? art_.chevL : art_.chevR;
        blit(ch, x - h * 0.65f, y - h * 0.35f, h, PAL_SIGN);
    }

    float dzEnd = FINISH - z_;
    if (dzEnd > 2.f && dzEnd < 80.f) {
        float near = 86.f / (dzEnd + 12.f);
        float x = 160.f - bendAt(z_ + dzEnd) * 84.f * near - (miss + lat_) * 22.f * near;
        float y = HORIZON + (gs::SCREEN_H - HORIZON - 24.f) * (7.f / (dzEnd + 7.f));
        float h = 20.f + 50.f * near;
        blit(art_.gate, x - h * 0.4f, y - h * 0.7f, h, PAL_GATE);
    }

    int bank = int(std::lround((lean_ * 0.5f + 0.5f) * 4.f));
    bank = std::max(0, std::min(4, bank));
    float bx = 128.f + (miss + lat_ * 0.4f) * 36.f;
    float by = 128.f;
    if (mode_ == Mode::Fail && tip_ >= 0.99f) {
        bx += std::sin(modeT_ * 16.f) * 12.f;
        by += std::sin(modeT_ * 8.f) * 5.f;
        blit(art_.tipped, bx, by, 72.f, PAL_KART);
    } else {
        blit(art_.kart[bank], bx, by, 78.f, PAL_KART);
    }
}

}  // namespace kartturn

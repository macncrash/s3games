#include "game/turn.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace biketurn {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float SPEED = 17.f;
constexpr float FINISH = 312.f;
constexpr float HORIZON = 78.f;
constexpr float MAX_LEAN = 1.f;
constexpr float LEAN_RATE = 2.1f;
constexpr int HORIZON_I = 78;

struct Corner {
    float a, b;
    float lean;
    const char* name;
};

const Corner CORNERS[3] = {
    {46.f, 96.f, -0.62f, "LEFT HAIRPIN"},
    {132.f, 186.f, 0.70f, "RIGHT SWEEP"},
    {220.f, 274.f, -0.78f, "LEFT KINK"},
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
    std::snprintf(result_, sizeof result_, "S3 BIKETURN  FAIL  unfinished");
    if (bot_) resetRide();
    else mode_ = Mode::Title;
    sys.apu.setMaster(0.75f);
}

void Game::resetRide() {
    mode_ = Mode::Ride;
    modeT_ = 0;
    z_ = 0;
    lean_ = 0;
    tip_ = 0;
    speed_ = SPEED;
    shake_ = 0;
    cleared_ = 0;
    passed_[0] = passed_[1] = passed_[2] = false;
    won_ = false;
    over_ = false;
}

void Game::finish(bool win) {
    won_ = win;
    mode_ = win ? Mode::Win : Mode::Crash;
    modeT_ = 0;
    shake_ = win ? 0 : 3;
    if (win) {
        std::snprintf(result_, sizeof result_, "S3 BIKETURN  PASS  three turns without tipping  %d/3", cleared_);
        sys_->apu.tone(0, 523.f, 0.25f);
        sys_->apu.tone(1, 659.f, 0.2f);
        sys_->apu.tone(2, 784.f, 0.18f);
    } else {
        std::snprintf(result_, sizeof result_, "S3 BIKETURN  FAIL  tipped  %d/3", cleared_);
        sys_->apu.tone(0, 90.f, 0.35f);
        sys_->apu.tone(1, 60.f, 0.3f);
        sys_->apu.tone(2, 0.f, 0.f);
    }
}

float Game::needAt(float z) const {
    float n = 0.f;
    for (const Corner& c : CORNERS) {
        float w = smooth(z, c.a - 8.f, c.b + 6.f);
        if (w > 0.f) n += c.lean * w;
    }
    return clampf(n, -MAX_LEAN, MAX_LEAN);
}

float Game::bendAt(float z) const {
    float b = 0.f;
    for (const Corner& c : CORNERS) b += c.lean * smooth(z, c.a - 4.f, c.b + 10.f);
    return b;
}

int Game::cornerAt(float z) const {
    for (int i = 0; i < 3; i++)
        if (z >= CORNERS[i].a && z < CORNERS[i].b) return i;
    return -1;
}

float Game::steer() const {
    if (bot_) {
        float look = needAt(z_ + 12.f);
        float err = look - lean_;
        return clampf(err * 3.4f, -1.f, 1.f);
    }
    const gs::Pad& p = sys_->pad;
    float a = p.axisX;
    if (p.down(gs::BTN_LEFT)) a -= 1.f;
    if (p.down(gs::BTN_RIGHT)) a += 1.f;
    return clampf(a, -1.f, 1.f);
}

void Game::ride(float dt) {
    float want = steer() * MAX_LEAN;
    float d = want - lean_;
    float step = LEAN_RATE * dt;
    if (std::fabs(d) <= step) lean_ = want;
    else lean_ += (d > 0.f ? step : -step);

    z_ += speed_ * dt;
    int ci = cornerAt(z_);
    float need = (ci >= 0) ? needAt(z_) : 0.f;
    float err = std::fabs(lean_ - need);
    if (ci >= 0) {
        if (err > 0.40f) tip_ += dt * (0.62f + (err - 0.40f) * 1.4f);
        else tip_ -= dt * 0.95f;
    } else {
        tip_ -= dt * 0.7f;
        if (std::fabs(lean_) > 0.88f) tip_ += dt * 0.35f;
    }
    tip_ = clampf(tip_, 0.f, 1.f);
    if (tip_ >= 1.f) {
        finish(false);
        return;
    }

    for (int i = 0; i < 3; i++) {
        if (!passed_[i] && z_ >= CORNERS[i].b) {
            passed_[i] = true;
            cleared_++;
            sys_->apu.tone(1, 440.f + float(i) * 80.f, 0.18f);
        }
    }
    if (z_ >= FINISH && cleared_ >= 3) finish(true);

    float rpm = 70.f + speed_ * 6.f + std::fabs(lean_) * 18.f;
    sys_->apu.tone(0, rpm, 0.06f);
    sys_->apu.tone(2, rpm * 0.5f, 0.03f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);

    if (mode_ == Mode::Title) {
        lean_ = std::sin(t_ * 1.3f) * 0.45f;
        if (start) resetRide();
    } else if (mode_ == Mode::Ride) {
        if (!bot_ && p.pressed(gs::BTN_START)) {
            held_ = Mode::Ride;
            mode_ = Mode::Pause;
        } else {
            ride(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) mode_ = held_;
        sys.apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Crash || mode_ == Mode::Win) {
        modeT_ += DT;
        sys.apu.tone(0, mode_ == Mode::Win ? 180.f : 50.f, modeT_ < 0.4f ? 0.12f : 0.f);
        if (bot_ && modeT_ > 1.1f) over_ = true;
        else if (!bot_ && start && modeT_ > 0.4f) {
            if (mode_ == Mode::Win) mode_ = Mode::Title;
            else resetRide();
        }
    }

    draw();
    if (!sys.headless) {
        if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
        else if (mode_ == Mode::Crash) sys.setLight(200, 30, 20);
        else if (tip_ > 0.55f) sys.setLight(200, 120, 20);
        else sys.setLight(30, 70, 160);
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
    float miss = lean_ - bendAt(z_ + 10.f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int yy = y;
        if (y > HORIZON_I) yy = std::max(0, std::min(gs::SCREEN_H - 1, y + shake));
        float u = y < HORIZON_I ? y / HORIZON : 1.f;
        int r = int(2 + 6 * u);
        int g = int(4 + 5 * u);
        int b = int(9 + 3 * (1.f - u));
        if (mode_ == Mode::Crash && modeT_ < 0.15f) r = std::min(15, r + 7);
        v.lineBackdrop[yy] = gs::rgb4(r, g, b);
        v.lineFog[y] = uint8_t(y < HORIZON_I ? (HORIZON_I - y) / 8 : 0);
        gs::RoadLine& road = v.road[yy];
        if (y <= HORIZON_I) {
            road.on = false;
            continue;
        }
        float dd = float(y - HORIZON_I);
        float dz = 520.f / dd;
        float wz = z_ + dz;
        float near = 90.f / (dz + 14.f);
        float cx = 160.f - bendAt(wz) * 78.f * near - miss * 36.f * near;
        road.on = true;
        road.cx = cx;
        road.hw = 10.f + dd * 1.15f;
        road.v = wz * 0.35f;
        road.pal = PAL_FIELD;
        road.band = (int(wz) & 8) ? 1 : 0;
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
        text(8, 20, "LEFT AND RIGHT LEAN THE BIKE", PAL_HUD);
        text(9, 22, "MATCH THE CORNER OR TIP", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) text(12, 25, "START TO RIDE", PAL_HUD);
        return;
    }

    char line[48];
    std::snprintf(line, sizeof line, "TURN %d/3", std::min(3, cleared_ + (mode_ == Mode::Win ? 0 : (cornerAt(z_) >= 0 ? 0 : 0))));
    int show = mode_ == Mode::Win ? 3 : cleared_;
    std::snprintf(line, sizeof line, "TURNS %d/3", show);
    text(1, 0, line, PAL_HUD);
    int ci = cornerAt(z_);
    if (mode_ == Mode::Ride && ci >= 0) text(14, 0, CORNERS[ci].name, tip_ > 0.45f ? 4 : 3);
    else if (mode_ == Mode::Ride) text(16, 0, "STRAIGHT", PAL_HUD);

    int slots = 17;
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
    if (mode_ == Mode::Crash) {
        text(10, 11, "TIPPED IT", 4);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(12, 14, "START RETRIES", PAL_HUD);
    }
    if (mode_ == Mode::Win) {
        text(6, 11, "THREE TURNS CLEAN", 5);
        if (!bot_ && (int(t_ * 2.f) & 1) == 0) text(13, 14, "START AGAIN", PAL_HUD);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    backdrop();
    hud();

    if (mode_ == Mode::Title || mode_ == Mode::Win) {
        image(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 28.f, PAL_TITLE);
        image(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 62.f, PAL_TITLE);
    }

    float miss = lean_ - bendAt(z_ + 8.f);
    for (int i = 0; i < 3; i++) {
        float dz = CORNERS[i].a - z_;
        if (dz < 4.f || dz > 70.f) continue;
        float near = 90.f / (dz + 14.f);
        float x = 160.f - bendAt(z_ + dz) * 78.f * near - miss * 20.f * near;
        float y = HORIZON + (gs::SCREEN_H - HORIZON - 20.f) * (6.f / (dz + 6.f));
        float h = 18.f + 40.f * near;
        const gs::Mipped& ch = CORNERS[i].lean < 0.f ? art_.chevL : art_.chevR;
        blit(ch, x - h * 0.7f, y - h * 0.4f, h, PAL_SIGN);
    }

    if (mode_ == Mode::Title && (int(t_ * 2.f) & 1)) {
        // Idle machine on the title, already drawn below.
    }

    int bank = int(std::lround((lean_ * 0.5f + 0.5f) * 4.f));
    bank = std::max(0, std::min(4, bank));
    float bx = 132.f + miss * 42.f;
    float by = 132.f;
    if (mode_ == Mode::Crash) {
        bx += std::sin(modeT_ * 18.f) * 10.f;
        by += std::sin(modeT_ * 9.f) * 6.f - 8.f;
        blit(art_.tumble, bx, by, 78.f, PAL_BIKE);
    } else {
        blit(art_.bike[bank], bx, by, 84.f, PAL_BIKE);
    }
}

}  // namespace biketurn

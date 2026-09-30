#include "game/heli.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace heliturn {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float SPEED = 52.f;
constexpr float END_L = 600.f;
constexpr float END_R = 668.f;
constexpr float MISS = 706.f;
constexpr float CLOCK = 16.5f;
constexpr float TIP = 1.18f;
constexpr float HOLD_Y = 56.f;
constexpr float K_TORQUE = 1.55f;
constexpr float K_STICK = 1.75f;
constexpr float K_SPRING = 1.15f;

struct Bend {
    float a, b;
    float mag;
    const char* name;
};

const Bend BENDS[3] = {
    {96.f, 206.f, -1.08f, "LEFT BEND"},
    {258.f, 378.f, 1.12f, "RIGHT BEND"},
    {428.f, 548.f, -1.06f, "LEFT HOOK"},
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

float pulse(float x, float a, float b) {
    if (x <= a || x >= b) return 0.f;
    float u = (x - a) / (b - a);
    return std::sin(u * 3.14159265f);
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.reset();
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = true;
    won_ = false;
    over_ = false;
    cleared_ = 0;
    t_ = 0;
    std::snprintf(result_, sizeof result_, "S3 HELITURN  FAIL  unfinished");
    if (bot_) resetLeg();
    else mode_ = Mode::Title;
    sys.apu.setMaster(0.72f);
}

void Game::resetLeg() {
    mode_ = Mode::Fly;
    modeT_ = 0;
    x_ = 24.f;
    y_ = HOLD_Y;
    vy_ = 0;
    roll_ = 0;
    rollV_ = 0;
    clock_ = 0;
    shake_ = 0;
    cleared_ = 0;
    passed_[0] = passed_[1] = passed_[2] = false;
    gated_ = false;
    won_ = false;
    over_ = false;
}

void Game::finish(bool win, const char* why) {
    if (mode_ != Mode::Fly) return;
    won_ = win;
    mode_ = win ? Mode::Win : Mode::Fail;
    modeT_ = 0;
    shake_ = win ? 0 : 4;
    if (win) {
        std::snprintf(result_, sizeof result_, "S3 HELITURN  PASS  three turns without tipping  %d/3", cleared_);
        sys_->apu.tone(0, 523.f, 0.22f);
        sys_->apu.tone(1, 659.f, 0.18f);
        sys_->apu.tone(2, 784.f, 0.16f);
    } else {
        std::snprintf(result_, sizeof result_, "S3 HELITURN  FAIL  %s  %d/3", why, cleared_);
        sys_->apu.tone(0, 80.f, 0.32f);
        sys_->apu.tone(1, 54.f, 0.26f);
        sys_->apu.tone(2, 0.f, 0.f);
    }
}

float Game::torqueAt(float x) const {
    float t = 0.f;
    for (const Bend& c : BENDS) t += c.mag * pulse(x, c.a, c.b);
    return t;
}

int Game::turnAt(float x) const {
    for (int i = 0; i < 3; i++)
        if (x >= BENDS[i].a && x < BENDS[i].b) return i;
    return -1;
}

float Game::steer() const {
    if (bot_) {
        float look = torqueAt(x_ + 28.f);
        float hold = -look * (K_TORQUE / K_STICK);
        float damp = clampf(-roll_ * 1.35f - rollV_ * 0.55f, -0.45f, 0.45f);
        return clampf(hold + damp, -1.f, 1.f);
    }
    const gs::Pad& p = sys_->pad;
    float a = p.axisX;
    if (p.down(gs::BTN_LEFT)) a -= 1.f;
    if (p.down(gs::BTN_RIGHT)) a += 1.f;
    return clampf(a, -1.f, 1.f);
}

float Game::climb() const {
    if (bot_) return clampf((HOLD_Y - y_) * 0.14f - vy_ * 0.22f, -1.f, 1.f);
    const gs::Pad& p = sys_->pad;
    float a = p.axisY;
    if (p.down(gs::BTN_UP) || p.down(gs::BTN_A)) a += 1.f;
    if (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) a -= 1.f;
    return clampf(a, -1.f, 1.f);
}

void Game::fly(float dt) {
    clock_ += dt;
    float torque = torqueAt(x_);
    float stick = steer();
    rollV_ += (torque * K_TORQUE + stick * K_STICK - roll_ * K_SPRING) * dt;
    rollV_ *= std::exp(-dt * 3.4f);
    roll_ += rollV_ * dt;
    roll_ = clampf(roll_, -1.6f, 1.6f);

    float col = climb();
    vy_ += col * 46.f * dt;
    vy_ *= std::exp(-dt * 2.4f);
    y_ += vy_ * dt;
    y_ = clampf(y_, 6.f, 120.f);
    x_ += SPEED * dt;

    if (std::fabs(roll_) >= TIP) {
        finish(false, "tipped");
        return;
    }
    if (y_ < 14.f) {
        finish(false, "tipped");
        return;
    }

    for (int i = 0; i < 3; i++) {
        if (!passed_[i] && x_ >= BENDS[i].b) {
            passed_[i] = true;
            cleared_++;
            sys_->apu.tone(1, 392.f + float(i) * 70.f, 0.16f);
        }
    }

    bool inGate = x_ >= END_L && x_ <= END_R && y_ >= 46.f && y_ <= 68.f && std::fabs(roll_) < 0.48f && cleared_ >= 3;
    if (inGate) gated_ = true;
    if (gated_ && x_ >= END_L + 18.f) {
        finish(true, "clean");
        return;
    }
    if (x_ > MISS || clock_ > CLOCK) {
        finish(false, "missed the end");
        return;
    }

    float rpm = 92.f + std::fabs(roll_) * 28.f + std::fabs(col) * 10.f;
    sys_->apu.tone(0, rpm, 0.05f);
    sys_->apu.tone(2, rpm * 0.5f, 0.025f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& p = sys.pad;
    bool start = p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A);

    if (mode_ == Mode::Title) {
        roll_ = std::sin(t_ * 1.1f) * 0.42f;
        y_ = HOLD_Y + std::sin(t_ * 1.7f) * 4.f;
        if (start) resetLeg();
    } else if (mode_ == Mode::Fly) {
        if (!bot_ && p.pressed(gs::BTN_START)) {
            held_ = Mode::Fly;
            mode_ = Mode::Pause;
        } else {
            fly(DT);
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) mode_ = held_;
        sys.apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Fail || mode_ == Mode::Win) {
        modeT_ += DT;
        sys.apu.tone(0, mode_ == Mode::Win ? 160.f : 48.f, modeT_ < 0.35f ? 0.1f : 0.f);
        if (bot_ && modeT_ > 0.9f) over_ = true;
        else if (!bot_ && start && modeT_ > 0.45f) {
            if (mode_ == Mode::Win) mode_ = Mode::Title;
            else resetLeg();
        }
    }

    draw();
    if (!sys.headless) {
        if (mode_ == Mode::Win) sys.setLight(40, 180, 70);
        else if (mode_ == Mode::Fail) sys.setLight(200, 30, 20);
        else if (std::fabs(roll_) > 0.85f) sys.setLight(200, 120, 20);
        else sys.setLight(30, 80, 150);
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

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    int shake = shake_ ? ((int(t_ * 60.f) & 1) ? shake_ : -shake_) : 0;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int yy = std::max(0, std::min(gs::SCREEN_H - 1, y + (y > 40 ? shake : 0)));
        float u = y / float(gs::SCREEN_H);
        int r = int(3 + 6 * (1.f - u));
        int g = int(6 + 4 * (1.f - u));
        int b = int(10 + 4 * (1.f - u));
        if (y > 168) {
            r = 7;
            g = 5;
            b = 3;
        }
        if (mode_ == Mode::Fail && modeT_ < 0.12f) r = std::min(15, r + 6);
        v.lineBackdrop[yy] = gs::rgb4(r, g, b);
        v.lineFog[y] = 0;
        v.road[y].on = false;
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
        text(7, 20, "BANK INTO THE THREE TURNS", PAL_HUD);
        text(8, 22, "UP AND DOWN HOLD HEIGHT", PAL_HUD);
        text(7, 23, "MISS THE END AND THE LEG FAILS", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) text(12, 25, "START TO FLY", PAL_HUD);
        return;
    }

    int show = mode_ == Mode::Win ? 3 : cleared_;
    char line[48];
    std::snprintf(line, sizeof line, "TURNS %d/3", show);
    text(1, 0, line, PAL_HUD);
    int left = int(std::ceil(std::max(0.f, CLOCK - clock_)));
    std::snprintf(line, sizeof line, "LEG %02d", left);
    text(32, 0, line, left <= 4 ? 4 : PAL_HUD);

    int ci = turnAt(x_);
    if (mode_ == Mode::Fly && ci >= 0) text(14, 0, BENDS[ci].name, std::fabs(roll_) > 0.8f ? 4 : 3);
    else if (mode_ == Mode::Fly && x_ >= END_L) text(15, 0, "THE END", 5);
    else if (mode_ == Mode::Fly) text(15, 0, "STRAIGHT", PAL_HUD);

    int slots = 17;
    int mark = int(std::lround((roll_ / TIP * 0.5f + 0.5f) * (slots - 1)));
    mark = std::max(0, std::min(slots - 1, mark));
    std::string bar = "BANK ";
    for (int i = 0; i < slots; i++) bar += (i == mark) ? '|' : (i == slots / 2 ? '+' : '-');
    text(1, 26, bar, std::fabs(roll_) > 0.85f ? 4 : PAL_HUD);

    if (mode_ == Mode::Pause) text(16, 12, "PAUSED", 4);
    if (mode_ == Mode::Fail) {
        bool missed = std::strstr(result_, "missed") != nullptr;
        text(missed ? 8 : 12, 11, missed ? "MISSED THE END" : "TIPPED IT", 4);
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
    sky();
    hud();

    float cam = (mode_ == Mode::Title) ? 0.f : x_ - 78.f;
    auto sx = [&](float wx) { return (wx - cam) * 1.05f + 36.f; };
    auto sy = [&](float alt) { return 168.f - alt * 1.35f; };

    for (int i = 0; i < 14; i++) {
        float wx = std::floor(cam / 48.f) * 48.f + float(i) * 48.f;
        float h = 36.f + std::fmod(std::fabs(wx) * 0.17f, 22.f);
        blit(art_.ridge, sx(wx) - 10.f, 168.f - h, h, PAL_RIDGE);
    }

    if (mode_ != Mode::Title) {
        for (int i = 0; i < 3; i++) {
            float wx = BENDS[i].a + 8.f;
            float px = sx(wx);
            if (px < -20.f || px > gs::SCREEN_W + 10.f) continue;
            const gs::Mipped& ch = BENDS[i].mag < 0.f ? art_.chevL : art_.chevR;
            blit(ch, px, sy(78.f), 22.f, PAL_SIGN);
        }
        float padX = sx((END_L + END_R) * 0.5f);
        if (padX > -30.f && padX < gs::SCREEN_W + 10.f) blit(art_.pad, padX - 16.f, sy(18.f), 16.f, PAL_PAD);
    }

    if (mode_ == Mode::Title || mode_ == Mode::Win) {
        image(art_.title, (gs::SCREEN_W - art_.title.w) * 0.5f, 26.f, PAL_TITLE);
        image(art_.sub, (gs::SCREEN_W - art_.sub.w) * 0.5f, 58.f, PAL_TITLE);
    }

    int bank = int(std::lround((roll_ / 0.7f * 0.5f + 0.5f) * 4.f));
    bank = std::max(0, std::min(4, bank));
    int rotor = int(t_ * 18.f) & 1;
    float hx = (mode_ == Mode::Title) ? 118.f : sx(x_) - 36.f;
    float hy = sy(y_) - 28.f;
    if (mode_ == Mode::Fail) {
        hx += std::sin(modeT_ * 16.f) * 8.f;
        hy += modeT_ * 18.f;
        blit(art_.wreck, hx, hy, 72.f, PAL_HELI);
    } else {
        blit(art_.heli[bank][rotor], hx, hy, 76.f, PAL_HELI);
    }
}

}  // namespace heliturn

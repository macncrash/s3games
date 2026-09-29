#include "game/plat.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace cliffplat {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kMark = 46.f;
constexpr float kPlat0 = 38.f;
constexpr float kPlat1 = 56.f;
constexpr float kNose = 1.6f;
constexpr float kRear = 1.8f;
constexpr float kDeck = 2.15f;
constexpr float kXTol = 0.48f;
constexpr float kHTol = 0.09f;
constexpr float kStop = 0.20f;
constexpr float kHoldNeed = 0.40f;
constexpr float kRival0 = -14.f;
constexpr float kRivalV = 1.72f;
constexpr float kPpm = 6.6f;
constexpr float kVppm = 16.f;
constexpr float kRail = 168.f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = clampf(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    begin();
    if (bot_) {
        mode_ = Mode::Run;
        s_ = 0.f;
        h_ = 0.42f;
        v_ = 2.4f;
        rival_ = kRival0;
        time_ = 0;
        hold_ = 0;
        idle_ = 0;
    }
}

void Game::begin() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    time_ = 0;
    s_ = 12.f;
    h_ = 0.7f;
    v_ = 0;
    rival_ = 2.f;
    hold_ = 0;
    idle_ = 0;
    shake_ = 0;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ == Mode::Title) {
        time_ += kDt;
        if (bot_ || sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Run;
            s_ = 0.f;
            h_ = 0.42f;
            v_ = 2.4f;
            rival_ = kRival0;
            time_ = 0;
            hold_ = idle_ = 0;
            won_ = over_ = false;
            sys.apu.tone(1, 420.f, 0.08f);
        }
    } else if (mode_ == Mode::Run) {
        float gas = 0, brake = 0, lift = 0;
        if (bot_) pilot(gas, brake, lift);
        else {
            if (sys.pad.down(gs::BTN_RIGHT) || sys.pad.accel > 0.2f) gas = sys.pad.accel > 0.2f ? sys.pad.accel : 1.f;
            if (sys.pad.down(gs::BTN_LEFT) || sys.pad.brake > 0.2f) brake = 1.f;
            if (sys.pad.down(gs::BTN_UP)) lift += 1.f;
            if (sys.pad.down(gs::BTN_DOWN)) lift -= 1.f;
            if (std::fabs(sys.pad.axisY) > 0.35f) lift += sys.pad.axisY;
            if (sys.pad.axisX > 0.35f) gas = std::max(gas, sys.pad.axisX);
            if (sys.pad.axisX < -0.35f) brake = 1.f;
        }
        physics(gas, brake, lift);
    } else if (sys.pad.pressed(gs::BTN_START) || sys.pad.pressed(gs::BTN_A)) {
        begin();
    }
    if (mode_ == Mode::Win) sys.apu.tone(0, 520.f + 40.f * std::sin(time_ * 6.f), 0.04f);
    draw();
}

void Game::pilot(float& gas, float& brake, float& lift) const {
    float err = kMark - s_;
    float vWant = 0.f;
    if (err > 0.55f) {
        float vStop = std::sqrt(std::max(0.f, 2.f * 8.2f * err));
        vWant = err > 11.f ? 8.0f : std::min(6.4f, std::max(0.62f, vStop * 0.88f));
    } else if (err < -0.32f) {
        vWant = -0.85f;
    }
    if (std::fabs(err) < 0.28f && std::fabs(v_) < 0.28f) vWant = 0.f;

    gas = 0.f;
    brake = 0.f;
    if (v_ > vWant + 0.12f) brake = 1.f;
    else if (v_ < vWant - 0.10f) {
        if (vWant >= 0.f) gas = 1.f;
        else brake = 0.f;
    }
    if (vWant < 0.f && v_ > 0.05f) {
        gas = 0.f;
        brake = 1.f;
    }
    if (vWant < 0.f && v_ <= 0.16f) {
        gas = 0.f;
        brake = 1.f;
    }
    lift = clampf((kDeck - h_) * 6.f, -1.f, 1.f);
}

void Game::physics(float gas, float brake, float lift) {
    gas = clampf(gas, 0.f, 1.f);
    brake = clampf(brake, 0.f, 1.f);
    lift = clampf(lift, -1.f, 1.f);
    time_ += kDt;

    bool reverse = brake > 0.4f && gas < 0.05f && v_ <= 0.18f && (kMark - s_) < -0.15f;
    float drive = gas * 6.6f;
    if (brake > 0.25f && v_ > 0.08f) drive = 0.f;
    float rev = reverse ? -2.5f : 0.f;
    float a = drive + rev - (brake > 0.25f && !reverse ? (v_ > 0.12f ? 10.8f : 3.2f) : 0.f) - 0.32f * v_;
    if (gas < 0.02f && brake < 0.05f) a -= 0.85f * v_;
    v_ += a * kDt;
    v_ = clampf(v_, -2.8f, 10.5f);
    if (std::fabs(v_) < 0.03f && gas < 0.02f && brake < 0.2f) v_ = 0.f;
    s_ += v_ * kDt;

    h_ += lift * 1.35f * kDt;
    h_ = clampf(h_, 0.15f, 3.1f);

    rival_ += kRivalV * kDt;
    shake_ = std::max(0.f, shake_ - kDt);

    if (s_ + kNose > kPlat1 + 0.15f) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        shake_ = 0.7f;
        sys_->apu.noiseBurst(0.4f, 460.f, 0.24f);
        sys_->setLight(180, 40, 30);
        return;
    }
    if (rival_ >= kMark) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        shake_ = 0.6f;
        sys_->apu.noiseBurst(0.35f, 300.f, 0.3f);
        sys_->setLight(160, 50, 20);
        return;
    }

    bool on = (s_ - kRear) >= kPlat0 - 0.4f && (s_ + kNose) <= kPlat1;
    bool at = std::fabs(s_ - kMark) <= kXTol;
    bool level = std::fabs(h_ - kDeck) <= kHTol;
    bool stopped = std::fabs(v_) <= kStop;
    if (on && at && level && stopped) {
        hold_ += kDt;
        idle_ = 0.f;
        if (hold_ >= kHoldNeed) {
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            v_ = 0;
            h_ = kDeck;
            s_ = kMark;
            sys_->rumble(0.18f, 0.05f, 100);
            sys_->setLight(40, 160, 70);
            sys_->apu.tone(1, 660.f, 0.1f);
        }
        return;
    }
    hold_ = 0.f;

    bool sitting = std::fabs(v_) < 0.04f && gas < 0.05f && std::fabs(lift) < 0.06f && brake < 0.15f;
    if (sitting && s_ > 6.f) idle_ += kDt;
    else idle_ = 0.f;
    if (idle_ > 0.7f) {
        won_ = false;
        over_ = true;
        mode_ = Mode::Lose;
        shake_ = 0.5f;
        sys_->apu.noiseBurst(0.3f, 220.f, 0.2f);
    }
}

void Game::sky() {
    uint16_t zen = gs::rgb4(2, 4, 8);
    uint16_t mid = gs::rgb4(6, 8, 12);
    uint16_t hor = gs::rgb4(13, 9, 6);
    uint16_t sea = gs::rgb4(2, 5, 8);
    if (mode_ == Mode::Lose) hor = lerpC(hor, gs::rgb4(12, 3, 2), 0.45f);
    if (mode_ == Mode::Win) hor = lerpC(hor, gs::rgb4(7, 12, 7), 0.4f);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float t = y / float(gs::SCREEN_H - 1);
        uint16_t c = t < 0.45f ? lerpC(zen, mid, t / 0.45f) : lerpC(mid, hor, (t - 0.45f) / 0.35f);
        if (y > 176) c = lerpC(sea, gs::rgb4(1, 3, 5), (y - 176) / 48.f);
        sys_->vdp.lineBackdrop[y] = c;
        sys_->vdp.lineFog[y] = 0;
        sys_->vdp.road[y].on = false;
    }
    sys_->vdp.A.enabled = false;
    sys_->vdp.B.enabled = false;
    sys_->vdp.hudEnabled = true;
    sys_->vdp.HUD.clear();
    sys_->vdp.setFogColor(gs::rgb4(5, 6, 8));
}

void Game::spr(const gs::Mipped& m, float cx, float feet, float ht, int pal, bool flip) {
    if (ht < 1.5f || m.h < 1) return;
    float w = ht * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 420.f)));
    s.h = int16_t(std::lround(clampf(ht, 1.f, 300.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet - s.h));
    s.img = m.pick(ht);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) { hud(20 - int(std::strlen(s)) / 2, row, s, pal); }

void Game::draw() {
    sky();
    sys_->vdp.clearSprites();
    float cam = (mode_ == Mode::Title) ? 18.f : s_ - 2.5f;
    float jx = (shake_ > 0.f) ? std::sin(time_ * 40.f) * 2.f : 0.f;
    auto sx = [&](float world) { return 78.f + (world - cam) * kPpm + jx; };
    auto floorY = [&](float h) { return kRail - h * kVppm; };

    for (int i = 0; i < 14; i++) {
        float wx = std::floor(cam / 4.f) * 4.f + i * 4.2f - 6.f;
        spr(art_.rock, sx(wx), 188.f, 92.f + (i % 3) * 10.f, PAL_ROCK, (i & 1) != 0);
    }

    float platHt = 36.f;
    spr(art_.plat, sx((kPlat0 + kPlat1) * 0.5f), floorY(kDeck) + platHt * 0.22f, platHt, PAL_PLAT);
    spr(art_.stripe, sx(kMark), floorY(kDeck) + 4.f, 26.f, PAL_PLAT);

    float gy = 48.f + std::sin(time_ * 1.4f) * 6.f;
    spr(art_.gull, 40.f + std::fmod(time_ * 28.f, 360.f) - 20.f, gy, 10.f, PAL_SEA);
    spr(art_.gull, 90.f + std::fmod(time_ * 18.f, 300.f), gy + 16.f, 8.f, PAL_SEA, true);

    spr(art_.rival, sx(rival_), floorY(0.55f), 40.f, PAL_RIVAL);
    spr(art_.cage, sx(s_), floorY(h_), 46.f, PAL_CAGE);

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(6, "S3 CLIFF PLAT", PAL_HUD);
        hudC(9, "STOP LEVEL WITH THE PLATFORM", PAL_HUD);
        hudC(11, "THE OTHER CREW IS THE CLOCK", PAL_HUD);
        hudC(16, "RIGHT GAS   LEFT BRAKE", PAL_HUD);
        hudC(17, "UP DOWN  RAISE THE CAGE", PAL_HUD);
        hudC(21, "START", PAL_HUD);
    } else {
        float left = std::max(0.f, (kMark - rival_) / kRivalV);
        std::snprintf(line, sizeof(line), "CREW %4.1f", left);
        hud(1, 1, line, left < 6.f ? PAL_BAD : PAL_HUD);
        std::snprintf(line, sizeof(line), "H %4.2f", h_);
        hud(28, 1, line, std::fabs(h_ - kDeck) <= kHTol ? PAL_GOOD : PAL_HUD);
        if (mode_ == Mode::Run) {
            if (hold_ > 0.05f) hudC(24, "HOLD LEVEL", PAL_GOOD);
            else if (s_ + kNose > kPlat0) hudC(24, "THE STRIPE", PAL_HUD);
            else hudC(24, "THE CLIFF", PAL_HUD);
        } else if (mode_ == Mode::Win) {
            hudC(12, "LEVEL", PAL_GOOD);
            hudC(14, "THE PLATFORM IS YOURS", PAL_HUD);
        } else {
            hudC(12, "THE STAND IS THEIRS", PAL_BAD);
            hudC(16, "START", PAL_HUD);
        }
    }
}

}  // namespace cliffplat

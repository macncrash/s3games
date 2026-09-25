#include "game/torpedo.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

#include "version.h"

namespace torpedo {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float SURFACE = 100.f;
constexpr float SUB_X = 54.f;
constexpr float LAUNCH_X = 112.f;
constexpr float TORP_V = 172.f;
constexpr float SUB_V = 68.f;
constexpr float SHIP_V0 = 42.f;
constexpr float SINK_DROP = 52.f;

gs::FMPatch motorPatch() {
    gs::FMPatch p;
    p.alg = 4;
    p.fb = 0.55f;
    p.op[0] = {0.5f, 0.8f, 0.05f, 0.45f, 0.9f, 0.35f, 0};
    p.op[1] = {1.0f, 1.0f, 0.04f, 0.4f, 0.85f, 0.3f, 0};
    p.op[2] = {2.0f, 0.22f, 0.03f, 0.35f, 0.4f, 0.25f, 0};
    p.op[3] = {0.5f, 0.3f, 0.02f, 0.3f, 0.6f, 0.2f, 0};
    p.vol = 0.16f;
    p.drive = 0.22f;
    p.tone = 640.f;
    return p;
}

gs::FMPatch brassPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.28f;
    p.op[0] = {1, 1, 0.01f, 0.16f, 0.75f, 0.14f, 0};
    p.op[1] = {2, 0.55f, 0.01f, 0.18f, 0.5f, 0.12f, 0};
    p.op[2] = {3, 0.28f, 0.02f, 0.22f, 0.4f, 0.14f, 0};
    p.op[3] = {1, 0.35f, 0.01f, 0.18f, 0.55f, 0.12f, 0};
    p.vol = 0.2f;
    p.drive = 0.08f;
    return p;
}

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto mix = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(mix(8), mix(4), mix(0));
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 3;
    if (flooding_) return 2;
    if (mode_ == Mode::Run || mode_ == Mode::Pause) return 1;
    return 0;
}

float Game::shipY() const {
    return SURFACE + 1.7f * std::sin(attackT_ * 1.15f) + drop_;
}

float Game::localY(float worldY) const { return worldY - shipY() + WL_Y; }

int Game::aimOf(float worldY) const {
    const float ly = localY(worldY);
    if (ly >= SAFE_Y0 && ly <= SAFE_Y1) return 0;
    if (ly > SAFE_Y1) return 1;
    return -1;
}

Game::Hit Game::probe(float wx, float wy) const {
    Hit best = Hit::None;
    const float ox[5] = {0, -2.2f, 2.2f, 0, 0};
    const float oy[5] = {0, 0, 0, -2.2f, 2.2f};
    for (int i = 0; i < 5; ++i) {
        const int lx = int(std::lround(wx + ox[i] - shipX_ + WL_X - 0.5f));
        const int ly = int(std::lround(wy + oy[i] - shipY() + WL_Y - 0.5f));
        const int c = art_.hull.get(lx, ly);
        Hit h = Hit::None;
        if (c == 5 || c == 12) h = Hit::Belly;
        else if (c == 4 || c == 6 || c == 11 || c == 13) h = Hit::Armor;
        if (h == Hit::Belly) return Hit::Belly;
        if (h == Hit::Armor) best = Hit::Armor;
    }
    return best;
}

void Game::say(const std::string& s, float t) {
    msg_ = s;
    msgT_ = t;
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        const int x = col + int(i);
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (!sys_ || h < 1.2f || m.h < 1) return;
    const float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 16.f * scale;
    x -= float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
    }
}

void Game::beginTitle() {
    mode_ = Mode::Title;
    attackT_ = 0;
    shipX_ = 214.f;
    shipV_ = 16.f;
    drop_ = 0;
    flooding_ = false;
    shotsLeft_ = 2;
    shotsFired_ = 0;
    killingShot_ = 0;
    cool_ = 0;
    shake_ = 0;
    hold_ = 0;
    doom_ = 0;
    over_ = false;
    won_ = false;
    subY_ = SURFACE + 36.f;
    msg_.clear();
    msgT_ = 0;
    end_.clear();
    torps_.clear();
    bits_.clear();
    fanStep_ = -1;
}

void Game::beginRun() {
    mode_ = Mode::Run;
    attackT_ = 0;
    shipX_ = 318.f;
    shipV_ = SHIP_V0;
    drop_ = 0;
    flooding_ = false;
    shotsLeft_ = 2;
    shotsFired_ = 0;
    killingShot_ = 0;
    cool_ = 0;
    shake_ = 0;
    hold_ = 0;
    doom_ = 0;
    over_ = false;
    won_ = false;
    subY_ = SURFACE + 86.f;  // under the keel; the green line is the belly
    say("PUT THE LINE IN THE RED BELLY", -1.f);
    end_.clear();
    torps_.clear();
    bits_.clear();
    fanStep_ = -1;
    if (sys_) {
        sys_->setLight(40, 90, 110);
        sys_->rumble(0, 0, 0);
    }
}

void Game::launch() {
    if (shotsLeft_ <= 0 || cool_ > 0 || flooding_) return;
    Torp t;
    t.x = LAUNCH_X;
    t.y = subY_;
    t.aim = aimOf(subY_);
    ++shotsFired_;
    t.shot = shotsFired_;
    --shotsLeft_;
    cool_ = 0.42f;
    torps_.push_back(t);
    shake_ = std::max(shake_, 0.25f);
    if (sys_) {
        sys_->apu.noiseBurst(0.38f, 2100.f, 0.12f);
        sys_->apu.keyOn(2, 180.f, 0.12f);
        sys_->rumble(0.35f, 0.15f, 90);
    }
}

void Game::clang() {
    shake_ = std::max(shake_, 0.45f);
    if (sys_) {
        sys_->apu.noiseBurst(0.28f, 700.f, 0.08f);
        sys_->apu.keyOn(2, 92.f, 0.16f);
        sys_->rumble(0.2f, 0.5f, 70);
    }
    if (shotsLeft_ > 0) say("TOO SHALLOW - GO DEEPER", 1.6f);
}

void Game::miss(const Torp& t) {
    if (t.aim > 0) {
        if (shotsLeft_ > 0) say("UNDER THE KEEL - COME UP", 1.6f);
    } else if (t.aim < 0) {
        if (shotsLeft_ > 0) say("TOO SHALLOW - GO DEEPER", 1.6f);
    } else if (shotsLeft_ > 0) {
        say("TOO LATE - SHE IS PAST", 1.6f);
    }
    if (sys_) sys_->apu.tone(1, 140.f, 0.04f);
    blip_ = 0.08f;
}

void Game::floodFrom(const Torp& t) {
    flooding_ = true;
    killingShot_ = t.shot;
    shipV_ *= 0.45f;
    shake_ = 1.f;
    say("FLOODING", -1.f);
    const float hx = t.x;
    const float hy = t.y;
    for (int i = 0; i < 8; ++i) {
        Bit b;
        b.x = hx + (i - 4) * 3.f;
        b.y = hy;
        b.vx = (i - 4) * 4.f;
        b.vy = -20.f - (i % 3) * 6.f;
        b.a = 1.f;
        b.kind = i < 2 ? 1 : 0;
        bits_.push_back(b);
    }
    if (sys_) {
        sys_->apu.noiseBurst(0.72f, 380.f, 0.55f);
        sys_->apu.keyOn(2, 60.f, 0.22f);
        sys_->rumble(0.9f, 0.4f, 280);
        sys_->setLight(180, 30, 20);
    }
}

void Game::fanfare() {
    fanStep_ = 0;
    fanT_ = 0;
}

void Game::fail(const char* why) {
    if (mode_ != Mode::Run) return;
    mode_ = Mode::Lost;
    hold_ = 0;
    end_ = why;
    won_ = false;
    fanStep_ = 0;
    fanT_ = 0;
    if (sys_) sys_->setLight(80, 20, 20);
}

void Game::play(float dt) {
    attackT_ += dt;
    if (cool_ > 0) cool_ -= dt;
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - dt);
    if (msgT_ > 0) {
        msgT_ -= dt;
        if (msgT_ <= 0) {
            msgT_ = 0;
            if (mode_ == Mode::Run && !flooding_ && shotsLeft_ > 0) say("PUT THE LINE IN THE RED BELLY", -1.f);
            else msg_.clear();
        }
    }

    const gs::Pad& pad = sys_->pad;
    bool pull = false;
    if (bot_) {
        const float target = shipY() + ((SAFE_Y0 + SAFE_Y1) * 0.5f - WL_Y);
        const float step = SUB_V * dt;
        if (std::fabs(subY_ - target) <= step) subY_ = target;
        else subY_ += std::copysign(step, target - subY_);
        const float bellyLeft = shipX_ + (BELLY_X0 - WL_X);
        if (!flooding_ && torps_.empty() && shotsLeft_ > 0 && cool_ <= 0 && std::fabs(subY_ - target) < 1.2f &&
            bellyLeft > LAUNCH_X + 8.f)
            pull = true;
    } else {
        float dir = 0;
        if (pad.down(gs::BTN_UP)) dir -= 1.f;
        if (pad.down(gs::BTN_DOWN)) dir += 1.f;
        subY_ += dir * SUB_V * dt;
        pull = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    }
    subY_ = std::clamp(subY_, SURFACE + 6.f, 210.f);
    if (pull) launch();

    shipX_ -= shipV_ * dt;
    if (flooding_) {
        shipV_ = std::max(0.f, shipV_ - 18.f * dt);
        if (drop_ < 62.f) drop_ += (7.f + drop_ * 0.2f) * dt;
        if ((int(attackT_ * 18.f) % 2) == 0) {
            Bit b;
            b.x = shipX_ + ((BELLY_X0 + BELLY_X1) * 0.5f - WL_X) + std::sin(attackT_ * 9.f) * 10.f;
            b.y = shipY() + ((BELLY_Y0 + BELLY_Y1) * 0.5f - WL_Y);
            b.vx = -6.f;
            b.vy = -28.f - 8.f * std::sin(attackT_ * 4.f);
            b.a = 1.f;
            b.kind = 0;
            bits_.push_back(b);
        }
    }

    for (int i = 0; i < int(torps_.size());) {
        Torp& t = torps_[i];
        t.x += TORP_V * dt;
        if (!flooding_) {
            const Hit h = probe(t.x, t.y);
            if (h == Hit::Belly) {
                floodFrom(t);
                torps_.erase(torps_.begin() + i);
                continue;
            }
            if (h == Hit::Armor) {
                Bit s;
                s.x = t.x;
                s.y = std::min(t.y, SURFACE + 2.f);
                s.vx = 0;
                s.vy = -8.f;
                s.a = 0.8f;
                s.kind = 1;
                bits_.push_back(s);
                clang();
                torps_.erase(torps_.begin() + i);
                continue;
            }
        }
        if (t.x > 348.f) {
            miss(t);
            torps_.erase(torps_.begin() + i);
            continue;
        }
        ++i;
    }

    // Funnel smoke while she still has a funnel above the water.
    if (drop_ < 36.f && (int(attackT_ * 12.f) % 3) == 0) {
        Bit b;
        b.x = shipX_ + (FUNNEL_X - WL_X);
        b.y = shipY() + (FUNNEL_Y - WL_Y);
        b.vx = -8.f;
        b.vy = -10.f;
        b.a = 0.9f;
        b.kind = 2;
        bits_.push_back(b);
    }
    // Prop wash.
    if ((int(attackT_ * 20.f) % 4) == 0) {
        Bit b;
        b.x = SUB_X - 36.f;
        b.y = subY_ + 2.f;
        b.vx = -12.f;
        b.vy = -6.f;
        b.a = 0.55f;
        b.kind = 0;
        bits_.push_back(b);
    }
    for (Bit& b : bits_) {
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.a -= dt * (b.kind == 2 ? 0.35f : 0.45f);
        if (b.kind == 0 && b.y < SURFACE - 2.f) b.a = 0;
    }
    bits_.erase(std::remove_if(bits_.begin(), bits_.end(), [](const Bit& b) { return b.a <= 0; }), bits_.end());
    if (bits_.size() > 96) bits_.erase(bits_.begin(), bits_.begin() + int(bits_.size() - 96));

    if (mode_ == Mode::Run && flooding_ && drop_ >= SINK_DROP) {
        mode_ = Mode::Won;
        won_ = true;
        hold_ = 0;
        end_ = "THE TARGET IS DOWN";
        fanfare();
        if (sys_) sys_->setLight(40, 160, 70);
    }

    if (mode_ == Mode::Run && !flooding_) {
        const bool live = !torps_.empty();
        const float bellyRight = shipX_ + (BELLY_X1 - WL_X);
        const bool spent = shotsLeft_ <= 0 && !live;
        const bool past = bellyRight < LAUNCH_X - 6.f && !live;
        if (spent || past) doom_ += dt;
        else doom_ = 0;
        if (doom_ > 0.4f) fail(spent ? "BOTH SHOTS GONE" : "SHE GOT AWAY");
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    v.hudEnabled = true;

    const float sh = shake_ > 0 ? std::sin(attackT_ * 97.f) * 3.2f * shake_ : 0.f;
    const uint16_t skyTop = gs::rgb4(2, 4, 9);
    const uint16_t skyHor = gs::rgb4(8, 11, 14);
    const uint16_t seaTop = gs::rgb4(3, 8, 12);
    const uint16_t seaDeep = gs::rgb4(1, 2, 5);
    const uint16_t sand = gs::rgb4(4, 4, 3);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < int(SURFACE) - 6) {
            v.lineBackdrop[y] = lerpC(skyTop, skyHor, y / (SURFACE - 6.f));
        } else if (y < int(SURFACE) + 3) {
            v.lineBackdrop[y] = gs::rgb4(12, 14, 13);
        } else {
            const float t = (y - SURFACE) / float(gs::SCREEN_H - SURFACE);
            v.lineBackdrop[y] = lerpC(seaTop, y > 200 ? sand : seaDeep, y > 200 ? (y - 200) / 24.f : t);
            if (y > 176) v.lineFog[y] = uint8_t(std::clamp(int((y - 176) / 6), 0, 8));
        }
    }

    // Far scenery first in the list so it sits behind (earlier sprites draw on top).
    const float sunX = 250.f + std::sin(attackT_ * 0.15f) * 4.f;
    spr(art_.sun, sunX, 28, 26, PAL_SKY);
    for (int i = 0; i < 3; ++i) {
        float x = std::fmod(40.f + i * 110.f - attackT_ * (6.f + i * 2.f), 380.f);
        if (x < 0) x += 380.f;
        spr(art_.cloud, x - 20.f, 18.f + (i == 1 ? 10.f : 0.f), 18.f + (i % 2) * 6.f, PAL_SKY);
    }
    for (int i = 0; i < 5; ++i) {
        float x = std::fmod(i * 78.f - attackT_ * 22.f, 360.f);
        if (x < 0) x += 360.f;
        spr(art_.wave, x, SURFACE - 2.f + std::sin(attackT_ * 2.f + i) * 1.5f, 10, PAL_FX);
    }

    const float sy = shipY() + sh * 0.4f;
    const int pose = drop_ > 34.f ? 2 : drop_ > 14.f ? 1 : 0;
    const int fog = std::clamp(int(drop_ / 5.f), 0, 13);
    spr(art_.ship[pose], shipX_, sy, float(SHIP_H), PAL_SHIP, false, fog);
    if (drop_ > 6.f) {
        const float ow = std::min(90.f, 18.f + drop_ * 1.3f);
        spr(art_.oil, shipX_, SURFACE + 1.f, 10.f * ow / 48.f, PAL_FX);
    }

    // Aim line. Green only when this depth clears the bow and meets the bilge.
    const int aim = aimOf(subY_);
    const int aimPal = aim == 0 ? PAL_GREEN : aim > 0 ? PAL_AMBER : PAL_RED;
    if (mode_ != Mode::Won) {
        for (float x = LAUNCH_X + 6.f; x < gs::SCREEN_W - 4.f; x += 12.f) spr(art_.dash, x, subY_, 2.2f, aimPal);
        const float bandTop = shipY() + (SAFE_Y0 - WL_Y);
        const float bandBot = shipY() + (SAFE_Y1 - WL_Y);
        spr(art_.tick, 14.f, (bandTop + bandBot) * 0.5f, std::max(8.f, bandBot - bandTop), PAL_GREEN);
    }

    spr(art_.sub, SUB_X + sh * 0.3f, subY_, 48, PAL_SUB);
    for (const Torp& t : torps_) spr(art_.torp, t.x - 14.f, t.y, 12, PAL_FX);

    for (const Bit& b : bits_) {
        if (b.kind == 1) spr(art_.splash, b.x, b.y, 16.f + (1.f - b.a) * 10.f, PAL_FX);
        else if (b.kind == 2) spr(art_.puff, b.x, b.y, 12.f + (1.f - b.a) * 14.f, PAL_FX, false, 2);
        else spr(art_.bubble, b.x, b.y, 6.f + b.a * 6.f, PAL_FX);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        text("S3 TORPEDO", 160, 16, 1.05f, PAL_RED);
        hudC(4, "TWO SHOTS", PAL_AMBER);
        hudC(5, "SINK THE RED BELLY", PAL_HUD);
        hudC(24, "ARROWS DEPTH    C FIRE", PAL_HUD);
        if (int(attackT_ * 2.f) % 2 == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 18, 1.1f, PAL_AMBER);
        hudC(4, "START RESUMES", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        text("SUNK", 160, 36, 1.45f, PAL_GREEN);
        hudC(8, end_.empty() ? "THE TARGET IS DOWN" : end_, PAL_HUD);
        std::snprintf(buf, sizeof buf, "SHOT %d OF 2", std::max(1, killingShot_));
        hudC(10, buf, PAL_AMBER);
        if (!bot_ && int(attackT_ * 2.f) % 2 == 0) hudC(13, "START", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        text("MISSED", 160, 18, 1.2f, PAL_RED);
        hudC(5, end_.empty() ? "SHE GOT AWAY" : end_, PAL_AMBER);
        if (!bot_ && int(attackT_ * 2.f) % 2 == 0) hudC(8, "START", PAL_HUD);
    } else {
        std::snprintf(buf, sizeof buf, "LEFT %d", shotsLeft_);
        hud(1, 1, buf, shotsLeft_ > 0 ? PAL_HUD : PAL_RED);
        if (aim == 0 && !flooding_) hud(28, 1, "BELLY", PAL_GREEN);
        else if (aim > 0 && !flooding_) hud(29, 1, "DEEP", PAL_AMBER);
        else if (!flooding_) hud(26, 1, "SHALLOW", PAL_RED);
        if (!msg_.empty()) hudC(3, msg_, flooding_ ? PAL_RED : aim == 0 ? PAL_GREEN : PAL_HUD);
    }

    if (blip_ > 0) {
        blip_ -= DT;
        if (blip_ <= 0) sys_->apu.tone(1, 0, 0);
    }
    if (engineOn_) {
        const float wow = 1.f + 0.035f * std::sin(attackT_ * 5.5f);
        const float rev = (mode_ == Mode::Run && !flooding_) ? 1.f : 0.7f;
        sys_->apu.setFreq(0, 44.f * wow * rev);
        sys_->apu.setVol(0, mode_ == Mode::Title ? 0.07f : flooding_ ? 0.05f : 0.13f);
    }
    if (mode_ == Mode::Title || mode_ == Mode::Run) {
        ping_ -= DT;
        if (ping_ <= 0) {
            sys_->apu.tone(0, mode_ == Mode::Run ? 820.f : 620.f, 0.045f);
            ping_ = mode_ == Mode::Run ? 1.15f : 1.8f;
            blip_ = 0.06f;
        }
    }
    if (fanStep_ >= 0) {
        static const float winN[] = {392.f, 494.f, 587.f, 784.f};
        static const float loseN[] = {220.f, 174.f, 130.f};
        const bool win = mode_ == Mode::Won;
        const float* notes = win ? winN : loseN;
        const int n = win ? 4 : 3;
        fanT_ += DT;
        if (fanT_ > 0.14f) {
            if (fanStep_ < n) sys_->apu.keyOn(1, notes[fanStep_], win ? 0.2f : 0.14f);
            else sys_->apu.keyOff(1);
            ++fanStep_;
            fanT_ = 0;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.22f, 0.14f);
    sys.apu.setPatch(0, motorPatch());
    sys.apu.setPatch(1, brassPatch());
    sys.apu.setPatch(2, brassPatch());
    sys.apu.keyOn(0, 44.f, 0.1f);
    engineOn_ = true;
    if (bot_) beginRun();
    else beginTitle();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        attackT_ += DT;
        shipX_ -= shipV_ * DT;
        if (shipX_ < -50.f) shipX_ = 390.f;
        subY_ = shipY() + ((SAFE_Y0 + SAFE_Y1) * 0.5f - WL_Y);
        if (!bot_ && pad.pressed(gs::BTN_START)) beginRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Run;
        else if (pad.pressed(gs::BTN_MODE)) beginTitle();
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        attackT_ += DT;
        hold_ += DT;
        if (hold_ > 1.05f) over_ = true;
        for (Bit& b : bits_) {
            b.y += b.vy * DT;
            b.a -= DT * 0.3f;
        }
        bits_.erase(std::remove_if(bits_.begin(), bits_.end(), [](const Bit& b) { return b.a <= 0; }), bits_.end());
        if (!bot_ && pad.pressed(gs::BTN_START)) beginRun();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) beginTitle();
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
    } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        beginTitle();
    } else {
        play(DT);
    }
    draw();
}

}  // namespace torpedo

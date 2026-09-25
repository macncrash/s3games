#include "game/clay.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace clay {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 146;
constexpr float FOCAL = 250.f;
constexpr float EYE = 1.6f;
constexpr float GRAV = 10.5f;
constexpr float CLAY_M = 2.3f;
constexpr float Y0 = 1.25f;
constexpr float Z0 = 11.4f;

// Trap oscillation for the round, then five wider match birds.
constexpr float HEAD[25] = {
    0.00f,  0.22f,  -0.18f, 0.35f,  -0.32f, 0.08f, -0.40f, 0.28f, -0.10f, 0.42f, -0.25f, 0.15f, -0.38f,
    0.05f,  0.33f,  -0.12f, 0.40f,  -0.30f, 0.18f, -0.22f, -0.62f, 0.70f, -0.15f, 0.55f, -0.78f,
};

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

struct Tree {
    float x, z;
    int kind;
};
constexpr Tree TREES[] = {
    {-9.4f, 16.5f, 0}, {-12.8f, 25.f, 1}, {-8.2f, 36.f, 0},
    {9.2f, 17.5f, 1},  {12.6f, 28.f, 0},  {8.4f, 40.f, 1},
};

}  // namespace

int Game::phase() const {
    if (mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Fly && bird_ >= 20) return 2;
    if (mode_ == Mode::Fly) return 1;
    return 0;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(13, 10, 7));
    mode_ = Mode::Title;
    beadX_ = 160;
    beadY_ = 102;
}

void Game::begin() {
    for (int i = 0; i < 25; i++) card_[i] = 0;
    broken_ = 0;
    matchBroken_ = 0;
    bird_ = 0;
    won_ = false;
    over_ = false;
    hit_ = false;
    shell_ = true;
    age_ = 0;
    release_ = -1;
    holdT_ = 0;
    bannerT_ = 0;
    overT_ = 0;
    flash_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    shardN_ = 0;
    beadX_ = 160;
    beadY_ = 102;
    mode_ = Mode::Ready;
    sys_->setLight(48, 22, 6);
}

void Game::armReady() {
    mode_ = Mode::Ready;
    release_ = -1;
    shell_ = true;
    age_ = 0;
    hit_ = false;
    shardN_ = 0;
}

void Game::blip(int ch, float freq, float hold) {
    sys_->apu.tone(ch, freq, 0.09f);
    beep_ = std::max(beep_, hold);
}

void Game::project(float x, float y, float z, float& sx, float& sy) const {
    z = std::max(0.85f, z);
    sx = 160.f + x * FOCAL / z;
    sy = float(HORIZON) - (y - EYE) * FOCAL / z;
}

void Game::sample(int index, float t, float& x, float& y, float& z, float& sx, float& sy) const {
    index = std::clamp(index, 0, 24);
    float sp = index >= 20 ? 20.5f : 17.5f;
    float lift = index >= 20 ? 18.5f : 16.5f;
    float vx = std::sin(HEAD[index]) * sp;
    float vz = std::cos(HEAD[index]) * sp;
    x = vx * t;
    y = Y0 + lift * t - 0.5f * GRAV * t * t;
    z = Z0 + vz * t;
    project(x, y, z, sx, sy);
}

void Game::launch() {
    age_ = 0;
    shell_ = true;
    hit_ = false;
    mode_ = Mode::Fly;
    float x, y, z, sx, sy;
    sample(bird_, 0, x, y, z, sx, sy);
    puff(sx, sy);
    sys_->apu.noiseBurst(0.16f, 1600.f, 0.05f);
}

void Game::puff(float sx, float sy) {
    shardN_ = 5;
    for (int i = 0; i < shardN_; i++) {
        float a = -0.6f + i * 0.3f;
        shards_[i] = {sx, sy, std::sin(a) * 36.f, -28.f - i * 6.f, 0.22f, i % 4};
    }
}

void Game::shatter(float sx, float sy) {
    shardN_ = 8;
    for (int i = 0; i < shardN_; i++) {
        float a = i * 6.2831853f / 8.f + 0.2f;
        float sp = 80.f + float(i % 3) * 36.f;
        shards_[i] = {sx, sy, std::cos(a) * sp, std::sin(a) * sp - 30.f, 0.48f, i % 4};
    }
}

void Game::ticks(float dt) {
    for (int i = 0; i < shardN_; i++) {
        Shard& s = shards_[i];
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.vy += 240.f * dt;
        s.life -= dt;
    }
}

bool Game::inPattern(float sx, float sy, float y, float z) const {
    if (age_ < 0.10f || y < 1.7f || z > 46.f) return false;
    if (sx < -8.f || sx > 328.f || sy < -8.f || sy > 216.f) return false;
    float h = CLAY_M * FOCAL / std::max(0.85f, z);
    float pattern = std::max(18.f, h * 0.70f);
    float dx = beadX_ - sx, dy = beadY_ - sy;
    return dx * dx + dy * dy <= pattern * pattern;
}

void Game::fire() {
    if (mode_ != Mode::Fly || !shell_) return;
    shell_ = false;
    flash_ = 0.12f;
    shake_ = 0.28f;
    sys_->rumble(0.8f, 0.45f, 70);
    sys_->apu.noiseBurst(0.40f, 5200.f, 0.07f);
    blip(0, 78.f, 0.10f);
    float x, y, z, sx, sy;
    sample(bird_, age_, x, y, z, sx, sy);
    if (!inPattern(sx, sy, y, z)) return;
    card_[bird_] = 1;
    broken_++;
    if (bird_ >= 20) matchBroken_++;
    hit_ = true;
    shatter(sx, sy);
    sys_->apu.noiseBurst(0.28f, 2400.f, 0.06f);
    blip(1, 640.f, 0.12f);
    shake_ = 0.4f;
    mode_ = Mode::Hold;
    holdT_ = 0.46f;
}

void Game::miss() {
    if (card_[bird_] == 0) card_[bird_] = 2;
    hit_ = false;
    mode_ = Mode::Hold;
    holdT_ = 0.42f;
    blip(1, 110.f, 0.16f);
}

bool Game::flightDone() const {
    float x, y, z, sx, sy;
    sample(bird_, age_, x, y, z, sx, sy);
    if (age_ > 2.30f) return true;
    if (age_ > 0.40f && y < 0.45f) return true;
    if (z > 54.f) return true;
    if (age_ > 0.55f && (sx < -30.f || sx > 350.f || sy < -24.f)) return true;
    return false;
}

void Game::steer(float dt) {
    if (bot_ && mode_ == Mode::Fly) {
        float x, y, z, sx, sy;
        sample(bird_, age_, x, y, z, sx, sy);
        float dx = sx - beadX_, dy = sy - beadY_;
        float d = std::hypot(dx, dy);
        float step = 3200.f * dt;
        if (d <= step) {
            beadX_ = sx;
            beadY_ = sy;
            d = 0;
        } else {
            beadX_ += dx / d * step;
            beadY_ += dy / d * step;
            d -= step;
        }
        beadX_ = clampf(beadX_, 6, 314);
        beadY_ = clampf(beadY_, 8, 210);
        if (shell_ && age_ >= 0.22f && y > 2.3f && z < 34.f && sx > 42.f && sx < 278.f && sy > 30.f && sy < 138.f &&
            d < 5.f)
            fire();
        return;
    }
    const gs::Pad& p = sys_->pad;
    float tx = 0, ty = 0;
    if (p.down(gs::BTN_LEFT)) tx -= 1;
    if (p.down(gs::BTN_RIGHT)) tx += 1;
    if (p.down(gs::BTN_UP)) ty -= 1;
    if (p.down(gs::BTN_DOWN)) ty += 1;
    if (std::fabs(p.axisX) > 0.12f) tx = p.axisX;
    float m = std::hypot(tx, ty);
    if (m > 1.f) {
        tx /= m;
        ty /= m;
    }
    if (mode_ == Mode::Ready || mode_ == Mode::Fly || mode_ == Mode::Title) {
        beadX_ = clampf(beadX_ + tx * 560.f * dt, 6, 314);
        beadY_ = clampf(beadY_ + ty * 560.f * dt, 8, 210);
    }
}

void Game::advance() {
    if (bird_ >= 24) {
        finish();
        return;
    }
    if (bird_ == 19) {
        mode_ = Mode::Banner;
        bannerT_ = 1.10f;
        blip(2, 520.f, 0.18f);
        return;
    }
    bird_++;
    armReady();
}

void Game::finish() {
    mode_ = Mode::Over;
    won_ = matchBroken_ == 5;
    over_ = true;
    overT_ = 0;
    fanStep_ = -1;
    if (won_) sys_->setLight(24, 96, 36);
    else {
        sys_->setLight(120, 18, 10);
        blip(1, 92.f, 0.22f);
    }
}

void Game::fanfare() {
    if (!won_) return;
    static const float notes[4] = {523.25f, 659.25f, 783.99f, 1046.5f};
    int step = int(overT_ / 0.18f);
    if (step != fanStep_ && step >= 0 && step < 4) {
        fanStep_ = step;
        blip(2, notes[step], 0.16f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    clock_ += DT;
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
    }
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT * 1.6f);
    if (flash_ > 0) flash_ = std::max(0.f, flash_ - DT);
    ticks(DT);

    const gs::Pad& pad = sys.pad;
    bool action = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO);
    bool start = pad.pressed(gs::BTN_START);
    bool back = pad.pressed(gs::BTN_MODE);

    if (back && !bot_) {
        if (mode_ == Mode::Pause) mode_ = held_;
        else if (mode_ == Mode::Title) sys.quit();
        else if (mode_ != Mode::Over) {
            held_ = mode_;
            mode_ = Mode::Pause;
        }
    }

    if (mode_ != Mode::Pause) {
        if (mode_ == Mode::Title) {
            titleT_ += DT;
            steer(DT);
            if (bot_ || start || action) begin();
        } else if (mode_ == Mode::Ready) {
            steer(DT);
            if (release_ < 0) {
                if (bot_ || action || start) {
                    release_ = 0.14f;
                    blip(2, 880.f, 0.10f);
                }
            } else {
                release_ -= DT;
                if (release_ <= 0) launch();
            }
        } else if (mode_ == Mode::Fly) {
            age_ += DT;
            steer(DT);
            if (!bot_ && action) fire();
            if (mode_ == Mode::Fly && flightDone()) miss();
        } else if (mode_ == Mode::Hold) {
            holdT_ -= DT;
            if (holdT_ <= 0) advance();
        } else if (mode_ == Mode::Banner) {
            bannerT_ -= DT;
            if (bannerT_ <= 0) {
                bird_ = 20;
                armReady();
            }
        } else if (mode_ == Mode::Over) {
            overT_ += DT;
            fanfare();
            if (!bot_ && start && overT_ > 0.55f) begin();
        }
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (h < 1.1f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    const float adv = 18.f * scale;
    float left = x - float(s.size()) * adv * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, left + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false);
    }
}

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

void Game::backdrop(float shx) {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < HORIZON) {
            float u = y / float(HORIZON - 1);
            float w = u * u;
            int r = std::clamp(int(3 + w * 12), 0, 15);
            int g = std::clamp(int(5 + u * 5), 0, 15);
            int b = std::clamp(int(12 - w * 6), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(y - HORIZON + 1);
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + shx;
        rd.hw = 7.f + row * 0.72f;
        rd.v = 4200.f / row;
        rd.pal = PAL_FIELD;
        rd.band = (int(rd.v / 48.f) & 1) ? 1 : 0;
        rd.style = 0;
        rd.left = rd.right = 0;
        int fog = row < 16.f ? int((16.f - row) / 3.f) : 0;
        v.lineFog[y] = uint8_t(std::clamp(fog, 0, 6));
        v.lineBackdrop[y] = gs::rgb4(2, 5, 2);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0, shy = 0;
    if (shake_ > 0) {
        shx = std::sin(clock_ * 90.f) * 5.f * shake_;
        shy = std::cos(clock_ * 76.f) * 3.f * shake_;
    }
    backdrop(shx);

    auto scenery = [&]() {
        float hsx, hsy;
        project(0, 0, 10.2f, hsx, hsy);
        spr(art_.house, hsx + shx, hsy + shy, 1.45f * FOCAL / 10.2f, PAL_HOUSE, false, 1, true);
        for (const Tree& t : TREES) {
            float sx, sy;
            project(t.x, 0, t.z, sx, sy);
            float th = (t.kind ? 5.4f : 4.6f) * FOCAL / t.z;
            int fog = std::clamp(int((t.z - 14.f) * 0.30f), 0, 9);
            spr(art_.tree[t.kind], sx + shx, sy + shy, th, PAL_TREE, t.x > 0, fog, true);
        }
        stamp(art_.hill, 160 + shx, float(HORIZON) - 2 + shy, 346, 54, PAL_TREE, 3);
        spr(art_.cloud, 86 + std::sin(clock_ * 0.17f) * 8.f + shx, 36 + shy, 30, PAL_SKY, false, 1);
        spr(art_.cloud, 196 + std::cos(clock_ * 0.13f) * 7.f + shx, 24 + shy, 38, PAL_SKY, true, 1);
        spr(art_.cloud, 286 + shx, 48 + shy, 22, PAL_SKY, false, 2);
        spr(art_.sun, 42 + shx, 30 + shy, 22.f + std::sin(clock_ * 1.7f) * 1.1f, PAL_SKY, false);
    };

    // Earlier sprites sit on top. Words and the bead, then the gun, then the bird.
    if (mode_ == Mode::Title) {
        text("S3 CLAY", 160 + shx, 52 + shy, 1.45f, PAL_GOLD);
        float loop = std::fmod(titleT_ * 0.40f, 1.30f);
        float x, y, z, sx, sy;
        sample(4, 0.16f + loop, x, y, z, sx, sy);
        if (y > 0.8f) {
            float h = std::max(8.f, CLAY_M * FOCAL / std::max(0.85f, z));
            int fr = int((titleT_ + loop) * 10.f) % 6;
            spr(art_.clay[fr], sx + shx, sy + shy, h, PAL_CLAY, false, 0);
        }
        for (int i = 0; i < 25; i++) {
            float px = 24.f + i * 11.0f;
            int fr = (i + int(clock_ * 5.f)) % 6;
            spr(art_.clay[fr], px, 20, 11, i >= 20 ? PAL_MATCH : PAL_CLAY, false);
        }
    } else if (mode_ == Mode::Ready && release_ >= 0) {
        text("PULL", 160, 78, 1.35f, PAL_GOLD);
    } else if (mode_ == Mode::Hold) {
        text(hit_ ? "BROKEN" : "LOST", 160, 64, 1.25f, hit_ ? PAL_GOLD : PAL_HUD);
    } else if (mode_ == Mode::Banner) {
        text("THE LAST FIVE", 160, 58, 0.92f, PAL_GOLD);
        text("ARE THE MATCH", 160, 84, 0.92f, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 70, 1.3f, PAL_HUD);
    } else if (mode_ == Mode::Over) {
        if (won_) {
            text("MATCH", 160, 52, 1.55f, PAL_GOLD);
            text("FIVE FOR FIVE", 160, 82, 0.78f, PAL_HUD);
        } else {
            text("LOST", 160, 52, 1.5f, PAL_HUD);
            text("THE MATCH", 160, 82, 1.0f, PAL_GOLD);
        }
    }

    if (flash_ > 0) spr(art_.flash, beadX_ + shx, beadY_ + shy, 18.f + (0.12f - flash_) * 160.f, PAL_FLASH, false);
    spr(art_.bead, beadX_ + shx, beadY_ + shy, 12, PAL_BEAD, false);

    for (int i = 0; i < shardN_; i++) {
        const Shard& s = shards_[i];
        if (s.life <= 0) continue;
        float h = 7.f + s.life * 16.f;
        int pal = (mode_ == Mode::Hold && hit_ && bird_ >= 20) ? PAL_MATCH : PAL_SHARD;
        spr(art_.shard[s.kind], s.x + shx, s.y + shy, h, pal, i & 1);
    }

    spr(art_.gun, 160.f + (beadX_ - 160.f) * 0.20f + shx, 234.f + shy, 62, PAL_GUN, false, 0, true);
    spr(art_.tuft, 22, 206, 18, PAL_TREE, false, 0, true);
    spr(art_.tuft, 48, 214, 14, PAL_TREE, true, 0, true);
    spr(art_.tuft, 276, 208, 16, PAL_TREE, false, 0, true);
    spr(art_.tuft, 304, 216, 13, PAL_TREE, true, 0, true);

    if (mode_ == Mode::Fly) {
        float x, y, z, sx, sy;
        sample(bird_, age_, x, y, z, sx, sy);
        float gsx, gsy;
        project(x, 0, z, gsx, gsy);
        float sw = std::max(6.f, CLAY_M * FOCAL / std::max(0.85f, z) * 0.75f);
        stamp(art_.shadow, gsx + shx, gsy + shy, sw, sw * 0.38f, PAL_HUD, 0, true);
        if (y > 0.35f) {
            float h = std::max(7.f, CLAY_M * FOCAL / std::max(0.85f, z));
            int fr = int(std::fabs(age_) * 11.f) % 6;
            int fog = std::clamp(int((z - 22.f) * 0.35f), 0, 6);
            spr(art_.clay[fr], sx + shx, sy + shy, h, bird_ >= 20 ? PAL_MATCH : PAL_CLAY, x < 0, fog);
        }
    }

    scenery();

    if (mode_ == Mode::Title) {
        hudC(22, "TWENTY-FIVE BIRDS", PAL_GOLD);
        hudC(23, "THE LAST FIVE ARE THE MATCH", PAL_HUD);
        hudC(25, "ARROWS AIM THE BEAD", PAL_HUD);
        hudC(26, "Z OR SPACE SHOOTS", PAL_HUD);
        hudC(27, "START", PAL_GOLD);
        return;
    }

    char line[40];
    std::snprintf(line, sizeof line, "BIRD %02d/25", birdNo());
    hud(1, 0, line, bird_ >= 20 ? PAL_GOLD : PAL_HUD);
    std::snprintf(line, sizeof line, "BROKEN %02d", broken_);
    hud(28, 0, line, PAL_HUD);
    std::snprintf(line, sizeof line, "MATCH %d/5", matchBroken_);
    hudC(1, line, bird_ >= 20 || mode_ == Mode::Banner || mode_ == Mode::Over ? PAL_GOLD : PAL_HUD);

    for (int set = 0; set < 5; set++) {
        std::string row = set == 4 ? "MATCH " : "SET " + std::to_string(set + 1) + " ";
        for (int k = 0; k < 5; k++) {
            int i = set * 5 + k;
            char c = '.';
            if (card_[i] == 1) c = '+';
            else if (card_[i] == 2) c = 'X';
            else if (i == bird_ && card_[i] == 0 &&
                     (mode_ == Mode::Ready || mode_ == Mode::Fly || mode_ == Mode::Hold))
                c = '*';
            row.push_back(c);
        }
        hud(1, 21 + set, row, set == 4 ? PAL_GOLD : PAL_HUD);
    }

    if (mode_ == Mode::Pause) hudC(27, "ESC", PAL_HUD);
    else if (mode_ == Mode::Over) hudC(27, won_ ? "START" : "START RETRIES", won_ ? PAL_GOLD : PAL_HUD);
    else if (mode_ == Mode::Banner) hudC(27, "FIVE BIRDS  ALL OF THEM", PAL_GOLD);
    else if (mode_ == Mode::Ready) hudC(27, release_ >= 0 ? "BIRD" : "Z CALLS PULL", PAL_HUD);
    else if (mode_ == Mode::Fly) hudC(27, shell_ ? "Z FIRES" : "SPENT", shell_ ? PAL_HUD : PAL_GOLD);
    else if (mode_ == Mode::Hold) hudC(27, hit_ ? "DEAD BIRD" : "GONE AWAY", hit_ ? PAL_GOLD : PAL_HUD);
}

}  // namespace clay

#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace lotwell {
namespace {

constexpr float kDt = 1.f;
constexpr float kWellX = 160.f;
constexpr float kWellY = 108.f;
constexpr float kWellR = 18.f;
constexpr float kBody = 12.f;
constexpr float kRamR = 12.f;
constexpr float kSpeed = 2.75f;
constexpr int kCracks = 4;
constexpr int kBanner = 70;

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Over) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Banner) return 1;
    return 0;
}

int Game::waveCount() const {
    if (wave_ <= 0) return 5;
    if (wave_ == 1) return 7;
    return 8;
}

float Game::waveSpeed() const {
    if (wave_ <= 0) return 0.62f;
    if (wave_ == 1) return 0.78f;
    return 0.92f;
}

void Game::blip(float freq, float vol) {
    if (!sys_) return;
    sys_->apu.tone(0, freq, vol);
    sys_->apu.tone(1, freq * 0.5f, vol * 0.4f);
}

void Game::begin() {
    wave_ = 0;
    cracks_ = 0;
    score_ = 0;
    spawned_ = 0;
    cool_ = 0;
    over_ = false;
    won_ = false;
    px_ = 160.f;
    py_ = 168.f;
    face_ = 1.f;
    rams_.clear();
    puffs_.clear();
    mode_ = Mode::Banner;
    banner_ = kBanner;
    if (sys_) sys_->apu.silence();
}

void Game::openWave() {
    spawned_ = 0;
    cool_ = 20;
    rams_.clear();
    mode_ = Mode::Play;
    blip(330.f + wave_ * 40.f, 0.05f);
}

void Game::spawnOne() {
    Ram r;
    int side = (spawned_ + wave_ * 3) & 3;
    int slot = (spawned_ * 5 + wave_ * 2) % 7;
    float along = 36.f + slot * 22.f;
    if (side == 0) {
        r.x = 16.f;
        r.y = 28.f + along;
    } else if (side == 1) {
        r.x = 304.f;
        r.y = 28.f + (154.f - along);
    } else if (side == 2) {
        r.y = 20.f;
        r.x = 40.f + along;
    } else {
        r.y = 204.f;
        r.x = 40.f + (154.f - along);
    }
    float dx = kWellX - r.x;
    float dy = kWellY - r.y;
    float d = std::hypot(dx, dy);
    if (d < 1.f) d = 1.f;
    float sp = waveSpeed();
    r.vx = dx / d * sp;
    r.vy = dy / d * sp;
    r.live = true;
    rams_.push_back(r);
    spawned_++;
}

void Game::tryMove(float dx, float dy) {
    auto blocked = [&](float x, float y) {
        if (x < 14.f || x > 306.f || y < 18.f || y > 206.f) return true;
        return std::hypot(x - kWellX, y - kWellY) < kWellR + 10.f;
    };
    float ox = px_, oy = py_;
    if (!blocked(px_ + dx, py_ + dy)) {
        px_ += dx;
        py_ += dy;
        return;
    }
    if (!blocked(px_ + dx, py_)) px_ += dx;
    if (!blocked(px_, py_ + dy)) py_ += dy;
    if (std::hypot(px_ - ox, py_ - oy) > 0.4f) return;
    px_ = ox;
    py_ = oy;
    float vx = px_ - kWellX;
    float vy = py_ - kWellY;
    float d = std::hypot(vx, vy);
    if (d < 1.f) d = 1.f;
    float tx = -vy / d * kSpeed;
    float ty = vx / d * kSpeed;
    if (dx * tx + dy * ty < 0.f) {
        tx = -tx;
        ty = -ty;
    }
    if (!blocked(px_ + tx, py_ + ty)) {
        px_ += tx;
        py_ += ty;
    }
}

void Game::driveHuman() {
    const gs::Pad& pad = sys_->pad;
    float x = 0, y = 0;
    if (pad.down(gs::BTN_LEFT)) x -= 1;
    if (pad.down(gs::BTN_RIGHT)) x += 1;
    if (pad.down(gs::BTN_UP)) y -= 1;
    if (pad.down(gs::BTN_DOWN)) y += 1;
    if (std::fabs(pad.axisX) > 0.28f) x = pad.axisX;
    if (std::fabs(pad.axisY) > 0.28f) y = -pad.axisY;
    float d = std::hypot(x, y);
    if (d > 0.05f) {
        face_ = x >= 0.f ? 1.f : -1.f;
        float s = kSpeed * std::min(d, 1.f);
        tryMove(x / d * s, y / d * s);
    }
}

void Game::driveBot() {
    const Ram* best = nullptr;
    float bestD = 1e9f;
    for (const Ram& r : rams_) {
        if (!r.live) continue;
        float d = std::hypot(r.x - kWellX, r.y - kWellY);
        if (d < bestD) {
            bestD = d;
            best = &r;
        }
    }
    float tx = kWellX;
    float ty = kWellY + 46.f;
    if (best) {
        tx = best->x;
        ty = best->y;
    }
    float dx = tx - px_;
    float dy = ty - py_;
    float d = std::hypot(dx, dy);
    if (d > 1.5f) {
        face_ = dx >= 0.f ? 1.f : -1.f;
        tryMove(dx / d * kSpeed, dy / d * kSpeed);
    }
}

void Game::shove(Ram& r) {
    r.live = false;
    score_ += 100 * (wave_ + 1);
    puffs_.push_back({r.x, r.y, 0});
    blip(520.f + wave_ * 30.f, 0.06f);
    sys_->apu.noiseBurst(0.12f, 900.f, 0.06f);
    sys_->rumble(0.2f, 0.35f, 40);
}

void Game::crackWell(Ram& r) {
    r.live = false;
    cracks_++;
    puffs_.push_back({kWellX, kWellY, 0});
    puffs_.push_back({r.x, r.y, 2});
    blip(90.f, 0.08f);
    sys_->apu.noiseBurst(0.28f, 280.f, 0.16f);
    sys_->rumble(0.7f, 0.4f, 120);
    if (cracks_ >= kCracks) lose();
}

void Game::clearWave() {
    if (wave_ >= 2) {
        win();
        return;
    }
    wave_++;
    mode_ = Mode::Banner;
    banner_ = kBanner;
    blip(440.f, 0.05f);
}

void Game::win() {
    mode_ = Mode::Victory;
    over_ = true;
    won_ = true;
    sys_->apu.tone(0, 523.f, 0.07f);
    sys_->apu.tone(1, 659.f, 0.05f);
    sys_->apu.tone(2, 784.f, 0.04f);
    sys_->rumble(0.4f, 0.7f, 180);
}

void Game::lose() {
    mode_ = Mode::Over;
    over_ = true;
    won_ = false;
    blip(70.f, 0.08f);
}

void Game::updatePlay() {
    if (cool_ > 0) cool_--;
    else if (spawned_ < waveCount()) {
        spawnOne();
        cool_ = wave_ == 0 ? 95 : (wave_ == 1 ? 85 : 78);
    }
    for (Ram& r : rams_) {
        if (!r.live) continue;
        r.x += r.vx * kDt;
        r.y += r.vy * kDt;
    }
    if (mode_ != Mode::Play) return;
    for (Ram& r : rams_) {
        if (!r.live) continue;
        if (std::hypot(r.x - px_, r.y - py_) < kBody + kRamR) shove(r);
    }
    if (mode_ != Mode::Play) return;
    for (Ram& r : rams_) {
        if (!r.live) continue;
        if (std::hypot(r.x - kWellX, r.y - kWellY) < kWellR + kRamR - 2.f) crackWell(r);
        if (mode_ != Mode::Play) return;
    }
    bool any = false;
    for (const Ram& r : rams_)
        if (r.live) any = true;
    if (!any && spawned_ >= waveCount()) clearWave();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ != Mode::Pause) t_ += 1.f;
    for (auto& p : puffs_) p.t += 1.f;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.t > 16.f; }), puffs_.end());
    sys.apu.tone(0, 0, 0);
    sys.apu.tone(1, 0, 0);
    sys.apu.tone(2, 0, 0);
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Banner) {
        if (--banner_ <= 0) openWave();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else {
            if (bot_) driveBot();
            else driveHuman();
            updatePlay();
        }
    } else if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) {
        begin();
    }
    draw();
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow, int fog) {
    if (h < 1.f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(m.h, 1));
    gs::Sprite s;
    s.w = int16_t(std::lround(std::clamp(w, 1.f, 480.f)));
    s.h = int16_t(std::lround(std::clamp(h, 1.f, 480.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
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

void Game::text(const std::string& s, float x, float y, float h, int pal) {
    if (s.empty()) return;
    const gs::Mipped& sample = art_.glyph['A' - 32];
    if (sample.h < 1) return;
    float adv = h * float(sample.w) / float(sample.h);
    float left = x - adv * float(s.size()) * 0.5f;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 32 || c > 126) continue;
        spr(art_.glyph[c - 32], left + (float(i) + 0.5f) * adv, y, h, pal, false);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.lineBackdrop[y] = gs::rgb4(1, 1, 3);
        v.lineFog[y] = 0;
        v.road[y].on = false;
    }
    const float stalls[][2] = {{48, 52}, {96, 52}, {224, 52}, {272, 52}, {48, 176}, {272, 176}};
    for (auto& s : stalls) {
        spr(art_.shadow, s[0], s[1] + 8, 8, PAL_FX, false, true);
        spr(art_.stall, s[0], s[1], 18, PAL_PROP, s[0] > 160, false, 2);
    }
    spr(art_.lamp, 28, 36, 36, PAL_LAMP, false);
    spr(art_.lamp, 292, 36, 36, PAL_LAMP, false);
    spr(art_.lamp, 28, 196, 36, PAL_LAMP, false);
    spr(art_.lamp, 292, 196, 36, PAL_LAMP, false);

    spr(art_.shadow, kWellX, kWellY + 16, 16, PAL_FX, false, true);
    float wellH = 52.f - cracks_ * 2.f;
    spr(art_.well, kWellX, kWellY, wellH, PAL_WELL, false);

    for (const Ram& r : rams_) {
        if (!r.live) continue;
        spr(art_.shadow, r.x, r.y + 8, 8, PAL_FX, false, true);
        spr(art_.ram, r.x, r.y, 22, PAL_RAM, r.vx < 0);
    }
    for (const Puff& p : puffs_) {
        float h = 18.f - p.t;
        if (h > 2.f) spr(art_.puff, p.x, p.y - p.t * 0.4f, h, PAL_FX, false);
    }
    spr(art_.shadow, px_, py_ + 8, 10, PAL_FX, false, true);
    spr(art_.watch, px_, py_, 26, PAL_WATCH, face_ < 0);

    hud(1, 1, "LOT WELL", PAL_GOLD);
    hud(28, 1, "WAVE " + std::to_string(wave_ + 1) + "/3", PAL_HUD);
    std::string stone = "WELL ";
    for (int i = 0; i < kCracks; i++) stone += (i < cracks_) ? "X" : "-";
    hud(1, 26, stone, cracks_ > 0 ? PAL_ALERT : PAL_OK);
    hud(28, 26, std::to_string(score_), PAL_HUD);

    if (mode_ == Mode::Title) {
        text("LOT WELL", 160, 78, 22, PAL_GOLD);
        text("KEEP THE WELL STANDING", 160, 108, 12, PAL_HUD);
        text("THREE WAVES", 160, 128, 12, PAL_DIM);
        text("START", 160, 156, 14, (int(t_) / 30) % 2 ? PAL_GOLD : PAL_HUD);
    } else if (mode_ == Mode::Banner) {
        text(std::string("WAVE ") + char('1' + wave_), 160, 64, 20, PAL_GOLD);
        text("HOLD THE STONE", 160, 88, 12, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 64, 18, PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        text("THE WELL STANDS", 160, 58, 16, PAL_OK);
        text("WATCH HELD", 160, 80, 12, PAL_GOLD);
    } else if (mode_ == Mode::Over) {
        text("THE WATCH IS OVER", 160, 58, 14, PAL_ALERT);
        text("THE WELL FELL", 160, 80, 12, PAL_DIM);
    }
}

}  // namespace lotwell

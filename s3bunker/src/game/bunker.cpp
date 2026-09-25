#include "bunker.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

#include "../version.h"

namespace bunker {
namespace {

constexpr float AIM_SPEED = 6.f;

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blip(int ch, float freq) {
    sys_->apu.tone(ch, freq, 0.06f);
    blipCh_ = ch;
    blipT_ = 7;
}

void Game::puff(float x, float y, int kind) {
    for (Mote& m : motes_) {
        if (m.life > 0) continue;
        int n = int(&m - motes_);
        m.x = x;
        m.y = y;
        m.vx = float((age_ + n) % 5 - 2) * 0.35f;
        m.vy = kind ? -0.8f : -0.25f;
        m.life = kind ? 0.22f : 0.4f;
        m.kind = kind;
        return;
    }
}

void Game::loadWave(int wave) {
    spawns_.clear();
    auto add = [&](int frame, int kind, int lane, float x) { spawns_.push_back({frame, kind, lane, x}); };
    if (wave <= 0) {
        add(70, WALKER, 1, 0.50f);
        add(180, WALKER, 0, 0.28f);
        add(290, WALKER, 2, 0.72f);
        add(400, WALKER, 1, 0.40f);
        add(520, WALKER, 0, 0.60f);
    } else if (wave == 1) {
        add(40, WALKER, 1, 0.46f);
        add(130, WALKER, 2, 0.74f);
        add(210, DUCKER, 0, 0.30f);
        add(310, WALKER, 0, 0.24f);
        add(400, DUCKER, 2, 0.68f);
        add(510, WALKER, 1, 0.52f);
    } else if (wave == 2) {
        add(30, WALKER, 0, 0.26f);
        add(100, WALKER, 2, 0.76f);
        add(170, DUCKER, 1, 0.48f);
        add(250, WALKER, 1, 0.38f);
        add(330, DUCKER, 0, 0.22f);
        add(410, WALKER, 2, 0.70f);
        add(500, DUCKER, 2, 0.80f);
        add(590, WALKER, 0, 0.34f);
    } else {
        add(36, WALKER, 1, 0.50f);
        add(110, DUCKER, 0, 0.28f);
        add(170, BREACHER, 1, 0.50f);
        add(260, WALKER, 2, 0.74f);
        add(340, DUCKER, 2, 0.66f);
        add(430, WALKER, 0, 0.24f);
        add(520, DUCKER, 1, 0.42f);
        add(630, WALKER, 1, 0.58f);
    }
}

void Game::spawnOne(const Spawn& s) {
    Enemy e;
    e.kind = s.kind;
    e.lane = std::clamp(s.lane, 0, 2);
    e.x = std::clamp(s.x, 0.08f, 0.92f);
    e.z = 1.f;
    e.alive = true;
    if (s.kind == DUCKER) {
        e.hp = 2;
        e.speed = 0.0048f;
        e.chew = 0.62f;
        e.points = 160;
    } else if (s.kind == BREACHER) {
        e.hp = 6;
        e.speed = 0.00225f;
        e.chew = 1.05f;
        e.points = 500;
    } else {
        e.hp = 1;
        e.speed = 0.0034f;
        e.chew = 0.28f;
        e.points = 100;
    }
    enemies_.push_back(e);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    bracing_ = false;
    score_ = 0;
    strain_ = 0;
    shake_ = 0;
    wave_ = 0;
    waveTime_ = 0;
    spawnIx_ = 0;
    calm_ = 0;
    cool_ = 0;
    shove_ = 0;
    flash_ = 0;
    wonAge_ = 0;
    aimX_ = SLIT_X + SLIT_W * 0.5f;
    aimY_ = SLIT_Y + SLIT_H * 0.5f;
    enemies_.clear();
    for (Mote& m : motes_) m.life = 0;
    loadWave(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->apu.noiseBurst(0.18f, 160.f, 0.12f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    aimX_ = SLIT_X + SLIT_W * 0.5f;
    aimY_ = SLIT_Y + SLIT_H * 0.5f;
    mode_ = Mode::Title;
    if (bot_) begin();
}

void Game::input() {
    if (bot_) return;
    const gs::Pad& pad = sys_->pad;
    const bool start = pad.pressed(gs::BTN_START);
    const bool back = pad.pressed(gs::BTN_MODE);
    if (back) {
        if (mode_ == Mode::Title) {
            if (sys_->hasHome()) sys_->eject();
            else sys_->quit();
        } else if (mode_ == Mode::Play) mode_ = Mode::Pause;
        else mode_ = Mode::Title;
        return;
    }
    if (!start) return;
    if (mode_ == Mode::Title || mode_ == Mode::Won || mode_ == Mode::Lost) begin();
    else if (mode_ == Mode::Play) mode_ = Mode::Pause;
    else if (mode_ == Mode::Pause) mode_ = Mode::Play;
}

void Game::titleTick() {
    const float cx = SLIT_X + SLIT_W * 0.5f;
    const float cy = SLIT_Y + SLIT_H * 0.5f;
    aimX_ = cx + std::sin(age_ * 0.02f) * 26.f;
    aimY_ = cy + std::sin(age_ * 0.013f) * 8.f;
}

bool Game::exposed(const Enemy& e) const {
    if (!e.alive) return false;
    if (e.kind == DUCKER) return e.z <= 0.04f;
    return true;
}

Game::Place Game::place(const Enemy& e) const {
    Place p;
    float near = 1.f - std::clamp(e.z, 0.f, 1.f);
    float base = e.kind == BREACHER ? 22.f : e.kind == DUCKER ? 14.f : 16.f;
    float grow = e.kind == BREACHER ? 34.f : 28.f;
    p.h = base + near * grow;
    p.x = float(SLIT_X) + 12.f + e.x * float(SLIT_W - 24);
    p.y = float(SLIT_Y) + float(SLIT_H) * 0.52f + float(e.lane - 1) * 13.f;
    return p;
}

float Game::hitR(const Place& p) const { return std::max(6.5f, p.h * 0.32f); }

void Game::kill(Enemy& e) {
    if (!e.alive) return;
    e.alive = false;
    int pts = e.points;
    if (e.kind != DUCKER && e.z > 0.15f) pts += 40;
    score_ += pts;
    Place p = place(e);
    puff(p.x, p.y, 0);
    blip(1, e.kind == BREACHER ? 196.f : 520.f);
}

void Game::shoot() {
    if (cool_ > 0 || bracing_) return;
    cool_ = 9;
    flash_ = 4;
    shake_ = std::max(shake_, 0.6f);
    sys_->apu.noiseBurst(0.32f, 2200.f, 0.05f);
    puff(aimX_, aimY_, 1);
    int hit = -1;
    float best = 1e9f;
    for (int i = 0; i < int(enemies_.size()); i++) {
        Enemy& e = enemies_[i];
        if (!exposed(e)) continue;
        Place p = place(e);
        float dx = aimX_ - p.x, dy = aimY_ - p.y;
        float r = hitR(p);
        float d2 = dx * dx + dy * dy;
        if (d2 > r * r) continue;
        float key = d2 + e.z * 20.f;
        if (key < best) {
            best = key;
            hit = i;
        }
    }
    if (hit < 0) return;
    Enemy& e = enemies_[hit];
    e.hp--;
    Place p = place(e);
    puff(p.x, p.y, 1);
    if (e.hp <= 0) kill(e);
}

void Game::shove() {
    int t = -1;
    float bestZ = 2.f;
    for (int i = 0; i < int(enemies_.size()); i++) {
        Enemy& e = enemies_[i];
        if (!e.alive || e.z > 0.04f) continue;
        if (e.z < bestZ) {
            bestZ = e.z;
            t = i;
        }
    }
    if (t < 0) return;
    Enemy& e = enemies_[t];
    e.hp--;
    e.z = 0.40f;
    shake_ = std::max(shake_, 1.6f);
    sys_->apu.noiseBurst(0.45f, 180.f, 0.16f);
    sys_->rumble(0.55f, 0.25f, 50);
    Place p = place(e);
    puff(p.x, BAR_Y, 0);
    if (e.hp <= 0) kill(e);
}

void Game::act(float& ax, float& ay, bool& fire, bool& brace) {
    ax = ay = 0;
    fire = false;
    brace = false;
    if (bot_) {
        int best = -1;
        float bestP = -1.f;
        for (int i = 0; i < int(enemies_.size()); i++) {
            const Enemy& e = enemies_[i];
            if (!e.alive) continue;
            float p = (1.f - e.z) * 100.f;
            if (e.z <= 0.04f) p += 400.f;
            if (exposed(e)) p += 220.f;
            if (e.kind == BREACHER) p += 70.f;
            if (e.kind == BREACHER && e.z < 0.55f) p += 260.f;
            if (e.kind == DUCKER && e.z < 0.3f) p += 40.f;
            if (p > bestP) {
                bestP = p;
                best = i;
            }
        }
        bool canHit = false;
        if (best >= 0) {
            Place p = place(enemies_[best]);
            float dx = p.x - aimX_;
            float dy = p.y - aimY_;
            ax = std::clamp(dx / AIM_SPEED, -1.f, 1.f);
            ay = std::clamp(dy / AIM_SPEED, -1.f, 1.f);
            float nx = std::fabs(dx) <= AIM_SPEED ? p.x : aimX_ + std::copysign(AIM_SPEED, dx);
            float ny = std::fabs(dy) <= AIM_SPEED ? p.y : aimY_ + std::copysign(AIM_SPEED, dy);
            float ex = nx - p.x, ey = ny - p.y;
            float r = hitR(p);
            canHit = exposed(enemies_[best]) && ex * ex + ey * ey <= r * r;
        }
        bool pressure = false;
        for (const Enemy& e : enemies_)
            if (e.alive && e.z <= 0.04f) pressure = true;
        if (pressure && !canHit) brace = true;
        else fire = canHit;
        return;
    }
    const gs::Pad& pad = sys_->pad;
    ax = pad.axisX;
    if (std::fabs(ax) < 0.12f) {
        ax = 0;
        if (pad.down(gs::BTN_LEFT)) ax -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) ax += 1.f;
    }
    if (pad.down(gs::BTN_UP)) ay -= 1.f;
    if (pad.down(gs::BTN_DOWN)) ay += 1.f;
    fire = pad.down(gs::BTN_A) || pad.down(gs::BTN_C);
    brace = pad.down(gs::BTN_B) || pad.down(gs::BTN_TURBO);
    if (brace) fire = false;
}

void Game::update() {
    if (calm_ > 0) {
        strain_ = std::max(0.f, strain_ - 0.45f);
        if (--calm_ == 0) {
            loadWave(wave_);
            waveTime_ = 0;
            spawnIx_ = 0;
        } else {
            shake_ *= 0.86f;
            return;
        }
    }
    while (spawnIx_ < int(spawns_.size()) && spawns_[spawnIx_].frame <= waveTime_) spawnOne(spawns_[spawnIx_++]);
    if (cool_ > 0) cool_--;
    float ax, ay;
    bool fire, brace;
    act(ax, ay, fire, brace);
    aimX_ = std::clamp(aimX_ + ax * AIM_SPEED, float(SLIT_X + 8), float(SLIT_X + SLIT_W - 8));
    aimY_ = std::clamp(aimY_ + ay * AIM_SPEED, float(SLIT_Y + 8), float(SLIT_Y + SLIT_H - 8));
    bracing_ = brace;
    if (brace) {
        shake_ = std::max(shake_, 0.3f);
        if (shove_ > 0) shove_--;
        if (shove_ == 0) {
            shove_ = 18;
            shove();
        }
    } else {
        shove_ = 0;
        if (fire) shoot();
    }
    bool hot = false;
    for (Enemy& e : enemies_) {
        if (!e.alive) continue;
        e.age++;
        if (e.z > 0) {
            e.z -= e.speed;
            if (e.z < 0) e.z = 0;
        }
        if (e.z <= 0.04f) {
            hot = true;
            strain_ += bracing_ ? e.chew * 0.12f : e.chew;
            if ((age_ + e.lane * 5) % 7 == 0) {
                Place p = place(e);
                puff(p.x, float(SLIT_Y + SLIT_H - 4), 1);
            }
        }
    }
    if (!hot) strain_ = std::max(0.f, strain_ - 0.08f);
    if (strain_ > 100.f) strain_ = 100.f;
    if (strain_ > 78.f && (age_ % 24) == 0) blip(2, 720.f);
    enemies_.erase(std::remove_if(enemies_.begin(), enemies_.end(), [](const Enemy& e) { return !e.alive; }), enemies_.end());
    shake_ *= 0.86f;
    if (shake_ < 0.12f) shake_ = 0;
    if (flash_ > 0) flash_--;
    waveTime_++;
    if (strain_ >= 100.f) {
        mode_ = Mode::Lost;
        won_ = false;
        over_ = true;
        wonAge_ = 0;
        bracing_ = false;
        shake_ = 2.f;
        sys_->apu.noiseBurst(0.6f, 90.f, 0.45f);
        blip(0, 98.f);
        sys_->rumble(1.f, 0.4f, 180);
        return;
    }
    if (spawnIx_ >= int(spawns_.size()) && enemies_.empty() && waveTime_ > 40) {
        score_ += 200;
        sys_->apu.noiseBurst(0.12f, 500.f, 0.08f);
        blip(2, 880.f);
        strain_ = std::max(0.f, strain_ - 22.f);
        if (wave_ >= 3) {
            mode_ = Mode::Won;
            won_ = true;
            over_ = true;
            wonAge_ = 0;
            score_ += 300;
            bracing_ = false;
            return;
        }
        wave_++;
        calm_ = 80;
    }
}

void Game::endTick() {
    wonAge_++;
    shake_ *= 0.9f;
    if (mode_ != Mode::Won) return;
    if (wonAge_ == 1) blip(0, 392.f);
    else if (wonAge_ == 12) blip(0, 523.f);
    else if (wonAge_ == 24) blip(0, 659.f);
    else if (wonAge_ == 40) blip(0, 784.f);
}

void Game::tickMotes() {
    for (Mote& m : motes_) {
        if (m.life <= 0) continue;
        m.life -= 1.f / 60.f;
        m.x += m.vx;
        m.y += m.vy;
    }
}

void Game::sprite(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f + shx_));
    s.y = int16_t(std::lround(cy - s.h * 0.5f + shy_));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::stamp(const gs::Mipped& m, float x, float y, float w, float h, int pal) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.x = int16_t(std::lround(x + shx_));
    s.y = int16_t(std::lround(y + shy_));
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        int x = col + i;
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    int n = int(std::strlen(s));
    hud(20 - n / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    int n = int(std::strlen(s));
    float adv = 18.f * scale;
    float left = x - n * adv * 0.5f;
    for (int i = 0; i < n; i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c < 33 || c > 126) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        sprite(g, left + float(i) * adv + adv * 0.5f, y, g.h * scale, pal, false, 0);
    }
}

void Game::draw() {
    int flick = ((age_ / 9) % 7 == 0) ? 10 : 13;
    sys_->vdp.setColor(PAL_ROOM * 16 + 8, gs::rgb4(15, flick, 4));
    sys_->vdp.setColor(PAL_ROOM * 16 + 9, gs::rgb4(15, 15, flick > 12 ? 12 : 8));
    shx_ = shy_ = 0;
    if (shake_ > 0.15f) {
        shx_ = std::sin(age_ * 1.9f) * std::min(shake_, 2.f);
        shy_ = std::cos(age_ * 2.4f) * std::min(shake_, 2.f) * 0.45f;
    }
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    for (int y = 0; y < gs::SCREEN_H; y++) v.lineBackdrop[y] = gs::rgb4(1, 1, 2);

    const bool open = mode_ == Mode::Lost;
    const float slide = open ? 68.f : 0.f;
    const bool hotSight = [&]() {
        for (const Enemy& e : enemies_) {
            if (!exposed(e)) continue;
            Place p = place(e);
            float dx = aimX_ - p.x, dy = aimY_ - p.y;
            if (dx * dx + dy * dy <= hitR(p) * hitR(p)) return true;
        }
        return false;
    }();

    if (flash_ > 0) sprite(art_.flash, aimX_, aimY_, 18, PAL_FX, false, 0);
    sprite(art_.sight, aimX_, aimY_, 18, hotSight ? PAL_ALERT : PAL_FX, false, 0);
    for (const Mote& m : motes_) {
        if (m.life <= 0) continue;
        float h = m.kind ? 6.f + m.life * 28.f : 8.f + m.life * 22.f;
        sprite(m.kind ? art_.spark : art_.dust, m.x, m.y, h, PAL_FX, false, 0);
    }
    float rx = 164.f + (aimX_ - (SLIT_X + SLIT_W * 0.5f)) * 0.18f;
    sprite(art_.rifle, rx, 198, 76, PAL_YOU, false, 0);
    // Banners sit on the steel, above the slit. Earlier sprites draw on top.
    if (mode_ == Mode::Title) {
        text("S3 BUNKER", 160, 36, 1.05f, PAL_AMBER);
        text("ONE DOOR", 160, 54, 0.8f, PAL_HUD);
    } else if (mode_ == Mode::Won) text("THE ROOM HELD", 160, 46, 0.95f, PAL_OK);
    else if (mode_ == Mode::Lost) text("THE DOOR OPENS", 160, 46, 0.85f, PAL_ALERT);

    if (strain_ > 28.f && !open) {
        float ch = 30.f + std::min(strain_, 80.f) * 0.32f;
        sprite(art_.crack, 160, 158, ch, PAL_FX, false, 0);
    }
    float barY = float(BAR_Y) + (bracing_ ? 3.f : 0.f);
    stamp(art_.bar, float(BAR_X) + slide, barY, float(BAR_W), float(BAR_H), PAL_DOOR);
    stamp(art_.door, float(DOOR_X) + slide, float(DOOR_Y), float(DOOR_W), float(DOOR_H), PAL_DOOR);

    if (mode_ == Mode::Title) {
        float bob = std::sin(age_ * 0.07f) * 4.f;
        int fr = (age_ / 10) & 1;
        sprite(art_.walker[fr], SLIT_X + SLIT_W * 0.62f + bob, SLIT_Y + SLIT_H * 0.58f, 30, PAL_FOE, false, 2);
        sprite(art_.walker[fr ^ 1], SLIT_X + 36, SLIT_Y + SLIT_H * 0.42f, 16, PAL_FOE, true, 10);
    } else {
        std::vector<int> order;
        order.reserve(enemies_.size());
        for (int i = 0; i < int(enemies_.size()); i++)
            if (enemies_[i].alive) order.push_back(i);
        std::sort(order.begin(), order.end(), [&](int a, int b) { return enemies_[a].z < enemies_[b].z; });
        for (int i : order) {
            const Enemy& e = enemies_[i];
            Place p = place(e);
            float h = p.h;
            int fog = int(std::clamp(e.z * 11.f, 0.f, 12.f));
            const gs::Mipped* img = &art_.walker[(e.age / 8) & 1];
            int pal = PAL_FOE;
            if (e.kind == DUCKER) {
                img = &art_.ducker[(e.age / 8) & 1];
                if (e.z > 0.04f) {
                    h = 11.f + (1.f - e.z) * 6.f;
                    fog = 8;
                }
            } else if (e.kind == BREACHER) {
                img = &art_.breacher[(e.age / 8) & 1];
                pal = PAL_BRUTE;
            }
            sprite(*img, p.x, p.y, h, pal, e.x < 0.45f, fog);
        }
    }

    char line[48];
    if (mode_ == Mode::Title) {
        hudC(25, "ONE ROOM. ONE DOOR.", PAL_HUD);
        hudC(26, "HOLD IT.", PAL_AMBER);
        hudC(27, "Z/C FIRE  X BRACE  ARROWS  ENTER", PAL_HUD);
        int n = int(std::strlen(S3_VERSION_STRING));
        hud(39 - n, 0, S3_VERSION_STRING, PAL_HUD);
    } else {
        int integ = std::clamp(int(std::lround(100.f - strain_)), 0, 100);
        int pips = (integ * 10 + 50) / 100;
        char bar[11];
        for (int i = 0; i < 10; i++) bar[i] = i < pips ? '#' : '-';
        bar[10] = 0;
        std::snprintf(line, sizeof line, "BAR %s", bar);
        hud(1, 0, line, integ < 35 ? PAL_ALERT : PAL_HUD);
        std::snprintf(line, sizeof line, "WAVE %d/4", std::min(wave_ + 1, 4));
        hud(30, 0, line, PAL_HUD);
        std::snprintf(line, sizeof line, "SCORE %d", score_);
        hud(1, 27, line, PAL_HUD);
        bool pressure = false;
        for (const Enemy& e : enemies_)
            if (e.alive && e.z <= 0.04f) pressure = true;
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_AMBER);
        else if (mode_ == Mode::Won || mode_ == Mode::Lost) hudC(26, "ENTER", PAL_HUD);
        else if (calm_ > 0) hudC(26, "HOLD", PAL_AMBER);
        else if (pressure) hudC(26, bracing_ ? "BRACING" : "BRACE", PAL_ALERT);
        else if (waveTime_ < 90 && wave_ == 0) hudC(26, "HOLD THE DOOR", PAL_HUD);
    }
    if (mode_ == Mode::Won) sys_->setLight(40, 160, 70);
    else if (strain_ > 70.f || mode_ == Mode::Lost) sys_->setLight(180, 30, 16);
    else sys_->setLight(150, 100, 40);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    age_++;
    if (blipT_ > 0 && --blipT_ == 0) sys.apu.tone(blipCh_, 0, 0);
    input();
    if (mode_ == Mode::Play) update();
    else if (mode_ == Mode::Won || mode_ == Mode::Lost) endTick();
    else if (mode_ == Mode::Title) titleTick();
    if (mode_ != Mode::Pause) tickMotes();
    draw();
}

}  // namespace bunker

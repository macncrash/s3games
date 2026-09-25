#include "game/gauntlet.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace gauntlet {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float LOOK = 168.f;
constexpr float START_Y = 1580.f;
constexpr float DOOR_Y = 200.f;
constexpr float MIN_X = 92.f;
constexpr float MAX_X = 228.f;
constexpr float MIN_Y = 230.f;
constexpr float MAX_Y = 1660.f;
constexpr float SPEED = 102.f;
constexpr int HEARTS = 6;
constexpr int LIVES = 3;

float clampf(float v, float a, float b) { return v < a ? a : v > b ? b : v; }

void faceVec(int f, float& x, float& y) {
    x = 0;
    y = 0;
    if (f == 0) y = -1;
    else if (f == 1) x = 1;
    else if (f == 2) y = 1;
    else x = -1;
}

int faceOf(float x, float y, int keep) {
    if (std::fabs(x) < 0.15f && std::fabs(y) < 0.15f) return keep;
    if (std::fabs(y) >= std::fabs(x)) return y < 0 ? 0 : 2;
    return x > 0 ? 1 : 3;
}

}  // namespace

int Game::left() const {
    int n = 0;
    for (const auto& e : enemies_)
        if (e.alive) n++;
    return n;
}

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Dead) return 1;
    return 0;
}

bool Game::hallClear() const { return left() == 0; }

int Game::prize(const Enemy& e) const {
    if (e.kind == Kind::Warden) return 800;
    if (e.kind == Kind::Archer) return 150;
    return e.maxHp >= 5 ? 250 : 100;
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.05f;
}

void Game::startFan() {
    if (fan_ < 0) {
        fan_ = 0;
        fanT_ = 0;
    }
}

void Game::fanStep(float dt) {
    if (fan_ < 0) return;
    fanT_ -= dt;
    if (fanT_ > 0) return;
    static const float notes[] = {392.f, 523.f, 659.f, 784.f, 1046.f};
    if (fan_ >= 5) {
        sys_->apu.tone(1, 0, 0);
        fan_ = -1;
        return;
    }
    sys_->apu.tone(1, notes[fan_], 0.07f);
    fanT_ = 0.13f;
    fan_++;
}

void Game::popup(float x, float y, int pts) {
    if (pops_.size() > 8) pops_.erase(pops_.begin());
    pops_.push_back({x, y, 0.8f, pts});
}

void Game::spawnSparks(float x, float y, int n, float speed) {
    for (int i = 0; i < n; i++) {
        float a = i * 6.2831853f / float(n);
        sparks_.push_back({x, y, std::cos(a) * speed, std::sin(a) * speed, 0.38f});
    }
    if (sparks_.size() > 48) sparks_.erase(sparks_.begin(), sparks_.begin() + int(sparks_.size() - 32));
}

void Game::spawnBolt(float x, float y, float tx, float ty, float speed, float rad) {
    float dx = tx - x, dy = ty - y;
    float d = std::hypot(dx, dy);
    if (d < 1.f) d = 1.f;
    dx /= d;
    dy /= d;
    bolts_.push_back({x + dx * 16.f, y + dy * 16.f, dx * speed, dy * speed, 2.4f, rad, true});
}

void Game::hurt(int dmg) {
    if (inv_ > 0 || mode_ != Mode::Play || won_) return;
    hearts_ -= dmg;
    inv_ = 1.05f;
    shake_ = 1.f;
    hurtFlash_ = 0.14f;
    sys_->apu.noiseBurst(0.42f, 520.f, 0.22f);
    sys_->apu.tone(0, 130.f, 0.08f);
    beep_ = 0.1f;
    sys_->rumble(0.7f, 0.35f, 110);
    if (hearts_ <= 0) {
        hearts_ = 0;
        lives_--;
        mode_ = Mode::Dead;
        deadT_ = 0.55f;
        swingT_ = 0;
    }
}

void Game::killEnemy(Enemy& e) {
    e.alive = false;
    e.flash = 0.2f;
    int pts = prize(e);
    score_ += pts;
    popup(e.x, e.y - 10.f, pts);
    spawnSparks(e.x, e.y, e.kind == Kind::Warden ? 12 : 6, e.kind == Kind::Warden ? 70.f : 48.f);
    sys_->apu.noiseBurst(e.kind == Kind::Warden ? 0.5f : 0.22f, e.kind == Kind::Warden ? 300.f : 1400.f, 0.16f);
    blip(e.kind == Kind::Warden ? 220.f : 620.f);
    if (e.kind == Kind::Warden) {
        keyOn_ = true;
        hasKey_ = false;
        keyX_ = clampf(e.x, 110.f, 210.f);
        keyY_ = clampf(e.y, 300.f, 1500.f);
        shake_ = 0.8f;
    }
}

void Game::swingHits() {
    if (swingT_ <= 0) return;
    float fx, fy;
    faceVec(facing_, fx, fy);
    auto inBlade = [&](float x, float y, float pad) {
        float dx = x - px_, dy = y - py_;
        float forward = dx * fx + dy * fy;
        float side = std::fabs(-dx * fy + dy * fx);
        return forward > 4.f && forward < 58.f + pad && side < 32.f + pad;
    };
    for (auto& e : enemies_) {
        if (!e.alive || e.struck == swingId_) continue;
        if (!inBlade(e.x, e.y, e.kind == Kind::Warden ? 6.f : 0.f)) continue;
        e.struck = swingId_;
        e.hp--;
        e.flash = 0.14f;
        e.stun = 0.18f;
        float dx = e.x - px_, dy = e.y - py_;
        float d = std::hypot(dx, dy);
        if (d < 1.f) {
            dx = fx;
            dy = fy;
            d = 1.f;
        }
        e.x += dx / d * 16.f;
        e.y += dy / d * 14.f;
        if (e.hp <= 0) killEnemy(e);
        else blip(480.f);
    }
    for (auto& b : bolts_) {
        if (!b.live) continue;
        if (!inBlade(b.x, b.y, b.rad)) continue;
        b.live = false;
        score_ += 25;
        popup(b.x, b.y, 25);
        spawnSparks(b.x, b.y, 3, 30.f);
        blip(880.f);
    }
}

void Game::resetLevel() {
    enemies_.clear();
    bolts_.clear();
    sparks_.clear();
    pops_.clear();
    foods_.clear();
    auto add = [&](Kind k, float x, float y, int hp) {
        Enemy e;
        e.kind = k;
        e.x = e.hx = x;
        e.y = e.hy = y;
        e.hp = e.maxHp = hp;
        e.shoot = 0.85f + float(enemies_.size()) * 0.04f;
        e.alive = true;
        enemies_.push_back(e);
    };
    add(Kind::Grunt, 112, 1470, 3);
    add(Kind::Grunt, 208, 1470, 3);
    add(Kind::Archer, 160, 1280, 2);
    add(Kind::Grunt, 108, 1060, 3);
    add(Kind::Grunt, 214, 1050, 3);
    add(Kind::Archer, 118, 870, 2);
    add(Kind::Archer, 206, 850, 2);
    add(Kind::Grunt, 160, 720, 5);
    add(Kind::Grunt, 114, 520, 3);
    add(Kind::Archer, 206, 510, 2);
    add(Kind::Warden, 160, 340, 10);
    foods_.push_back({160, 1380, false});
    foods_.push_back({140, 980, false});
    foods_.push_back({190, 600, false});
    px_ = 160;
    py_ = START_Y;
    facing_ = 0;
    score_ = 0;
    lives_ = LIVES;
    hearts_ = HEARTS;
    hasKey_ = false;
    keyOn_ = false;
    cp_ = START_Y;
    swingT_ = swingCd_ = inv_ = 0;
    shake_ = hurtFlash_ = 0;
    stall_ = bash_ = 0;
    anchorX_ = px_;
    anchorY_ = py_;
    playT_ = 0;
    moving_ = false;
    over_ = false;
    won_ = false;
    fan_ = -1;
}

void Game::begin() {
    resetLevel();
    mode_ = Mode::Play;
    inv_ = 0.35f;
}

void Game::revive() {
    px_ = 160;
    py_ = cp_;
    hearts_ = HEARTS;
    inv_ = 1.5f;
    facing_ = 0;
    swingT_ = 0;
    bolts_.clear();
    mode_ = Mode::Play;
    anchorX_ = px_;
    anchorY_ = py_;
    stall_ = 0;
}

void Game::intent(float& mx, float& my, bool& swing, float& faceX, float& faceY) {
    mx = my = faceX = faceY = 0;
    swing = false;
    if (!bot_) {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) mx -= 1;
        if (pad.down(gs::BTN_RIGHT)) mx += 1;
        if (pad.down(gs::BTN_UP)) my -= 1;
        if (pad.down(gs::BTN_DOWN)) my += 1;
        if (std::fabs(pad.axisX) > 0.15f) mx = pad.axisX;
        faceX = mx;
        faceY = my;
        swing = pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_B) || pad.down(gs::BTN_TURBO);
        return;
    }

    int ahead = -1, any = -1, food = -1;
    float bestA = 1e9f, bestN = 1e9f, bestF = 1e9f;
    float nearD = 1e9f;
    for (int i = 0; i < int(enemies_.size()); i++) {
        const Enemy& e = enemies_[i];
        if (!e.alive) continue;
        float d = std::hypot(e.x - px_, e.y - py_);
        if (d < nearD) nearD = d;
        if (d < bestN) {
            bestN = d;
            any = i;
        }
        if (e.y - py_ < 48.f && d < bestA) {
            bestA = d;
            ahead = i;
        }
    }
    if (hearts_ <= 2 && nearD > 80.f) {
        for (int i = 0; i < int(foods_.size()); i++) {
            if (foods_[i].taken) continue;
            float d = std::hypot(foods_[i].x - px_, foods_[i].y - py_);
            if (d < 240.f && d < bestF) {
                bestF = d;
                food = i;
            }
        }
    }

    int ti = ahead >= 0 ? ahead : any;
    if (food >= 0 && (ti < 0 || bestF + 30.f < (ahead >= 0 ? bestA : bestN))) ti = -2;

    if (ti >= 0) {
        const Enemy& e = enemies_[ti];
        float dx = e.x - px_, dy = e.y - py_;
        float d = std::hypot(dx, dy);
        if (d < 1.f) d = 1.f;
        float nx = dx / d, ny = dy / d;
        faceX = nx;
        faceY = ny;
        float ideal = e.kind == Kind::Warden ? 40.f : 36.f;
        float retreat = e.kind == Kind::Warden ? 30.f : 26.f;
        if (bash_ > 0) {
            ideal = 8.f;
            retreat = -100.f;
        }
        if (d > ideal + 4.f) {
            mx = nx;
            my = ny;
        } else if (d < retreat) {
            mx = -nx;
            my = -ny;
        }
        if (d < 56.f) swing = true;
    } else if (ti == -2) {
        float dx = foods_[food].x - px_, dy = foods_[food].y - py_;
        float d = std::hypot(dx, dy);
        if (d > 1.f) {
            mx = dx / d;
            my = dy / d;
        }
        faceX = mx;
        faceY = my;
    } else {
        float gx = 160.f, gy = MIN_Y + 8.f;
        if (keyOn_ && !hasKey_) {
            gx = keyX_;
            gy = keyY_;
        }
        float dx = gx - px_, dy = gy - py_;
        float d = std::hypot(dx, dy);
        if (d > 1.f) {
            mx = dx / d;
            my = dy / d;
        }
        faceX = mx;
        faceY = my;
        if (nearD < 56.f) swing = true;
    }

    float dodgeX = 0, dodgeY = 0, dodgeW = 0;
    for (const auto& b : bolts_) {
        if (!b.live) continue;
        float dx = px_ - b.x, dy = py_ - b.y;
        float dist = std::hypot(dx, dy);
        float spd = std::hypot(b.vx, b.vy);
        if (spd < 1.f || dist < 1.f) continue;
        float closing = b.vx * dx + b.vy * dy;
        if (closing <= 0) continue;
        float tti = dist / spd;
        if (tti > 0.5f || dist > 96.f) continue;
        float pxn = -b.vy / spd, pyn = b.vx / spd;
        auto room = [&](float sgn) {
            float nx = px_ + pxn * sgn * 26.f;
            return std::min(nx - MIN_X, MAX_X - nx);
        };
        float sgn = room(1.f) >= room(-1.f) ? 1.f : -1.f;
        float w = (0.5f - tti) / 0.5f;
        dodgeX += pxn * sgn * w;
        dodgeY += pyn * sgn * w;
        dodgeW += w;
    }
    if (dodgeW > 0.2f) {
        mx = mx * 0.2f + dodgeX;
        my = my * 0.2f + dodgeY;
    }
    float m = std::hypot(mx, my);
    if (m > 1.f) {
        mx /= m;
        my /= m;
    }
}

void Game::tickFx(float dt) {
    for (auto& s : sparks_) {
        s.x += s.vx * dt;
        s.y += s.vy * dt;
        s.vx *= 0.98f;
        s.vy *= 0.98f;
        s.t -= dt;
    }
    while (!sparks_.empty() && sparks_.front().t <= 0) sparks_.erase(sparks_.begin());
    for (auto& p : pops_) {
        p.y -= 16.f * dt;
        p.t -= dt;
    }
    while (!pops_.empty() && pops_.front().t <= 0) pops_.erase(pops_.begin());
}

void Game::update(float dt) {
    playT_ += dt;
    if (inv_ > 0) inv_ -= dt;
    for (auto& e : enemies_) {
        if (e.flash > 0) e.flash -= dt;
        if (e.stun > 0) e.stun -= dt;
    }

    float mx, my, faceX, faceY;
    bool wantSwing = false;
    intent(mx, my, wantSwing, faceX, faceY);
    if (swingT_ <= 0) facing_ = faceOf(faceX, faceY, facing_);
    moving_ = std::hypot(mx, my) > 0.1f;
    px_ = clampf(px_ + mx * SPEED * dt, MIN_X, MAX_X);
    py_ = clampf(py_ + my * SPEED * dt, MIN_Y, MAX_Y);
    if (wantSwing && swingCd_ <= 0 && swingT_ <= 0) {
        swingT_ = 0.12f;
        swingCd_ = 0.22f;
        swingId_++;
        sys_->apu.noiseBurst(0.12f, 1800.f, 0.05f);
        blip(740.f);
    }
    if (swingT_ > 0) swingT_ -= dt;
    else if (swingCd_ > 0) swingCd_ -= dt;
    swingHits();

    for (auto& e : enemies_) {
        if (!e.alive) continue;
        float dx = px_ - e.x, dy = py_ - e.y;
        float d = std::hypot(dx, dy);
        if (d < 1.f) d = 1.f;
        float aggro = e.kind == Kind::Warden ? 115.f : 155.f;
        if (!e.alert && d < aggro && py_ < e.y + 180.f) e.alert = true;
        if (e.alert && e.kind != Kind::Warden && d > 380.f) e.alert = false;
        if (e.stun > 0) continue;
        if (!e.alert) {
            float hx = e.hx - e.x, hy = e.hy - e.y;
            float hd = std::hypot(hx, hy);
            if (hd > 2.f) {
                e.x += hx / hd * 28.f * dt;
                e.y += hy / hd * 28.f * dt;
            }
            continue;
        }
        float speed = e.kind == Kind::Warden ? 44.f : e.kind == Kind::Archer ? 40.f : (e.maxHp >= 5 ? 40.f : 52.f);
        float touch = e.kind == Kind::Warden ? 22.f : e.kind == Kind::Archer ? 15.f : 17.f;
        if (e.kind == Kind::Warden) {
            float hx = e.x - e.hx, hy = e.y - e.hy;
            float hd = std::hypot(hx, hy);
            if (hd > 120.f) {
                e.x -= hx / hd * speed * dt;
                e.y -= hy / hd * speed * dt;
            } else if (d > touch) {
                e.x += dx / d * speed * dt;
                e.y += dy / d * speed * dt;
            }
            e.shoot -= dt;
            if (e.shoot <= 0 && d > 46.f && d < 230.f && bolts_.size() < 14) {
                spawnBolt(e.x, e.y, px_, py_, 76.f, 7.f);
                e.shoot = 1.75f;
            }
        } else if (e.kind == Kind::Archer) {
            if (d < 58.f) {
                e.x -= dx / d * speed * dt;
                e.y -= dy / d * speed * dt;
            }
            e.shoot -= dt;
            if (e.shoot <= 0 && d > 72.f && d < 300.f && bolts_.size() < 14) {
                spawnBolt(e.x, e.y, px_, py_, 96.f, 5.f);
                e.shoot = 1.5f;
            }
        } else if (d > touch) {
            e.x += dx / d * speed * dt;
            e.y += dy / d * speed * dt;
        }
    }

    for (size_t i = 0; i < enemies_.size(); i++) {
        if (!enemies_[i].alive) continue;
        for (size_t j = i + 1; j < enemies_.size(); j++) {
            if (!enemies_[j].alive) continue;
            float dx = enemies_[j].x - enemies_[i].x;
            float dy = enemies_[j].y - enemies_[i].y;
            float d = std::hypot(dx, dy);
            float need = 22.f;
            if (d < 0.01f) {
                enemies_[j].x += 1.f;
                continue;
            }
            if (d < need) {
                float p = (need - d) * 0.5f;
                enemies_[i].x -= dx / d * p;
                enemies_[i].y -= dy / d * p;
                enemies_[j].x += dx / d * p;
                enemies_[j].y += dy / d * p;
            }
        }
    }
    for (auto& e : enemies_) {
        if (!e.alive) continue;
        e.x = clampf(e.x, 104.f, 216.f);
        e.y = clampf(e.y, 300.f, 1700.f);
    }

    // Sword lands before contact, so a connecting swing stuns them out of the hit.
    for (auto& e : enemies_) {
        if (!e.alive || e.stun > 0 || !e.alert) continue;
        float d = std::hypot(e.x - px_, e.y - py_);
        float touch = e.kind == Kind::Warden ? 22.f : e.kind == Kind::Archer ? 15.f : 17.f;
        if (d < touch) hurt(1);
        if (mode_ != Mode::Play) break;
    }

    for (auto& b : bolts_) {
        if (!b.live) continue;
        b.x += b.vx * dt;
        b.y += b.vy * dt;
        b.life -= dt;
        if (b.life <= 0 || b.x < 28.f || b.x > 292.f || b.y < 140.f || b.y > 1720.f) {
            b.live = false;
            continue;
        }
        if (std::hypot(b.x - px_, b.y - py_) < b.rad + 8.f) {
            b.live = false;
            hurt(1);
        }
    }
    {
        size_t w = 0;
        for (size_t r = 0; r < bolts_.size(); r++)
            if (bolts_[r].live) bolts_[w++] = bolts_[r];
        bolts_.resize(w);
    }

    for (auto& f : foods_) {
        if (f.taken) continue;
        if (std::hypot(f.x - px_, f.y - py_) < 18.f) {
            f.taken = true;
            hearts_ = std::min(HEARTS, hearts_ + 2);
            blip(660.f);
            popup(f.x, f.y, 0);
        }
    }
    if (keyOn_ && !hasKey_ && std::hypot(keyX_ - px_, keyY_ - py_) < 22.f) {
        hasKey_ = true;
        keyOn_ = false;
        score_ += 50;
        blip(990.f);
        popup(px_, py_ - 8.f, 50);
    }
    if (py_ < 1360.f) cp_ = std::min(cp_, 1440.f);
    if (py_ < 1000.f) cp_ = std::min(cp_, 1160.f);
    if (py_ < 640.f) cp_ = std::min(cp_, 800.f);

    if (hallClear() && hasKey_ && py_ < 252.f && std::fabs(px_ - 160.f) < 72.f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Victory;
        score_ += 500;
        shake_ = 0.7f;
        startFan();
        spawnSparks(160.f, DOOR_Y + 20.f, 14, 60.f);
        sys_->rumble(0.45f, 0.85f, 200);
        sys_->setLight(200, 140, 40);
    }

    if (bot_) {
        float moved = std::hypot(px_ - anchorX_, py_ - anchorY_);
        if (moved > 8.f) {
            anchorX_ = px_;
            anchorY_ = py_;
            stall_ = 0;
        } else stall_ += dt;
        if (stall_ > 3.f) {
            bash_ = 0.85f;
            stall_ = 0;
        }
        if (bash_ > 0) bash_ -= dt;
    }
    tickFx(dt);
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

void Game::blit(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(std::max(1, m.h));
    gs::Sprite s;
    s.w = int16_t(std::lround(clampf(w, 1.f, 400.f)));
    s.h = int16_t(std::lround(clampf(h, 1.f, 400.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float cam = py_ - LOOK;
    camI_ = int(std::floor(cam));
    shx_ = int(std::lround(shake_ > 0 ? std::sin(t_ * 50.f) * 3.f * shake_ : 0.f));
    shy_ = int(std::lround(shake_ > 0 ? std::cos(t_ * 47.f) * 2.f * shake_ : 0.f));
    // Floor sample stays locked to world pixels while the whole view shakes.
    v.B.scroll(shx_, camI_ - shy_);

    uint16_t fogC = gs::rgb4(2, 2, 4);
    int add = 0;
    if (hurtFlash_ > 0) {
        fogC = gs::rgb4(10, 2, 2);
        add = 5;
    }
    if (mode_ == Mode::Over) {
        fogC = gs::rgb4(1, 1, 2);
        add = 8;
    } else if (mode_ == Mode::Victory) fogC = gs::rgb4(10, 7, 3);
    v.setFogColor(fogC);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int depth = y < 84 ? (84 - y) / 16 : 0;
        if (mode_ == Mode::Victory) depth /= 2;
        v.lineFog[y] = uint8_t(std::min(16, depth + add));
        v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
    }

    auto sx = [&](float x) { return x + float(shx_); };
    auto sy = [&](float y) { return y - float(camI_) + float(shy_); };
    auto world = [&](const gs::Mipped& m, float x, float y, float h, int pal, bool flip = false, bool shadow = false) {
        blit(m, sx(x), sy(y), h, pal, flip, shadow);
    };

    auto text = [&](const std::string& s, float x, float y, float scale, int pal) {
        const float adv = 16.f * scale;
        float w = float(s.size()) * adv;
        x -= w * 0.5f;
        for (size_t i = 0; i < s.size(); i++) {
            unsigned char c = static_cast<unsigned char>(s[i]);
            if (c <= 32 || c >= 128) continue;
            const gs::Mipped& g = art_.glyph[c - 32];
            blit(g, x + float(i) * adv + g.w * scale * 0.5f, y, g.h * scale, pal, false);
        }
    };

    if (mode_ == Mode::Title) text("S3 GAUNTLET", 160, 20, 1.05f, PAL_GOLD);
    else if (mode_ == Mode::Pause) text("PAUSE", 160, 96, 1.2f, PAL_WHITE);
    else if (mode_ == Mode::Dead) text(lives_ > 0 ? "FALLEN" : "FALLEN", 160, 78, 1.15f, PAL_RED);
    else if (mode_ == Mode::Over) text("THE CORRIDOR HOLDS", 160, 78, 0.7f, PAL_RED);
    else if (mode_ == Mode::Victory) text("THE DOOR OPENS", 160, 36, 0.85f, PAL_GOLD);

    for (auto& p : pops_) {
        if (p.t <= 0 || p.pts <= 0) continue;
        char buf[16];
        std::snprintf(buf, sizeof buf, "+%d", p.pts);
        text(buf, sx(p.x), sy(p.y), 0.55f, PAL_GOLD);
    }
    for (const auto& s : sparks_) {
        if (s.t <= 0) continue;
        world(art_.spark, s.x, s.y, 6.f + s.t * 18.f, PAL_FX);
    }
    if (swingT_ > 0.01f) {
        int sf = facing_;
        bool flip = false;
        float ox = 0, oy = 0;
        if (sf == 3) {
            sf = 1;
            flip = true;
            ox = -22;
        } else if (sf == 1) ox = 22;
        else if (sf == 2) oy = 20;
        else oy = -20;
        world(art_.slash[sf], px_ + ox, py_ + oy, 38, PAL_FX, flip);
    }
    for (const auto& b : bolts_)
        if (b.live) world(art_.bolt, b.x, b.y, b.rad > 6.f ? 18.f : 12.f, PAL_FX);

    struct Who {
        float y;
        int i;
    };
    Who who[16];
    int wn = 0;
    for (int i = 0; i < int(enemies_.size()) && wn < 15; i++)
        if (enemies_[i].alive || enemies_[i].flash > 0) who[wn++] = {enemies_[i].y, i};
    who[wn++] = {py_, -1};
    std::sort(who, who + wn, [](const Who& a, const Who& b) { return a.y > b.y; });
    for (int n = 0; n < wn; n++) {
        if (who[n].i < 0) {
            bool blink = inv_ > 0 && (int(inv_ * 24.f) & 1);
            if (!blink) {
                int face = facing_;
                bool flip = false;
                if (face == 3) {
                    face = 1;
                    flip = true;
                }
                int step = (swingT_ > 0) ? 1 : (moving_ ? (int(t_ * 9.f) & 1) : 0);
                float bob = mode_ == Mode::Title ? std::sin(t_ * 2.f) * 1.5f : 0.f;
                world(art_.shadow, px_, py_ + 16.f, 16, PAL_KNIGHT, false, true);
                world(art_.knight[face][step], px_, py_ + bob, 48, PAL_KNIGHT, flip);
            }
            continue;
        }
        const Enemy& e = enemies_[who[n].i];
        if (e.flash > 0 && (int(e.flash * 40.f) & 1) && e.alive) continue;
        bool flip = px_ < e.x;
        int step = e.alert ? ((int(t_ * 7.f) + who[n].i) & 1) : 0;
        float bob = std::sin(t_ * 3.f + float(who[n].i)) * 1.2f;
        if (e.kind == Kind::Warden) {
            world(art_.shadow, e.x, e.y + 20.f, 22, PAL_WARDEN, false, true);
            world(art_.warden[step], e.x, e.y + bob, 64, PAL_WARDEN, flip);
        } else if (e.kind == Kind::Archer) {
            world(art_.shadow, e.x, e.y + 14.f, 16, PAL_ARCHER, false, true);
            world(art_.archer[step], e.x, e.y + bob, 46, PAL_ARCHER, flip);
        } else {
            float h = e.maxHp >= 5 ? 54.f : 44.f;
            world(art_.shadow, e.x, e.y + 14.f, e.maxHp >= 5 ? 20.f : 15.f, PAL_GRUNT, false, true);
            world(art_.grunt[step], e.x, e.y + bob, h, PAL_GRUNT, flip);
        }
    }

    for (const auto& f : foods_) {
        if (f.taken) continue;
        float bob = std::sin(t_ * 4.f + f.y) * 1.5f;
        world(art_.flask, f.x, f.y + bob, 20, PAL_ITEM);
    }
    if (keyOn_) {
        float bob = std::sin(t_ * 5.f) * 2.f;
        world(art_.key, keyX_, keyY_ + bob, 16, PAL_ITEM);
    }

    world(mode_ == Mode::Victory ? art_.doorOpen : art_.door, 160, DOOR_Y, 112, PAL_DOOR);

    static const float torchY[] = {360, 530, 700, 870, 1040, 1210, 1380, 1550};
    for (int i = 0; i < 8; i++) {
        int fr = (int(t_ * 9.f) + i) & 1;
        world(art_.torch[fr], 44, torchY[i], 30, PAL_ITEM);
        world(art_.torch[fr], 276, torchY[i], 30, PAL_ITEM);
    }
    static const float pillarY[] = {480, 760, 1100};
    for (float y : pillarY) {
        world(art_.pillar, 62, y, 56, PAL_FLOOR);
        world(art_.pillar, 258, y, 56, PAL_FLOOR);
    }
    world(art_.skull, 150, 900, 14, PAL_ITEM);
    world(art_.skull, 186, 640, 14, PAL_ITEM);

    for (int i = 0; i < HEARTS; i++) {
        int pal = (mode_ == Mode::Title) ? PAL_DIM : (i < hearts_ ? PAL_RED : PAL_DIM);
        if (mode_ != Mode::Title) v.HUD.set(i, 0, gs::entry(art_.heart, pal));
    }
    char buf[48];
    if (mode_ == Mode::Title) {
        hud(40 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_DIM);
        hudC(26, "THE CORRIDOR, THEN THE DOOR", PAL_GOLD);
        hudC(27, "ARROWS MOVE    C SWINGS", PAL_WHITE);
    } else if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Dead) {
        std::snprintf(buf, sizeof buf, "X%d", std::max(0, lives_));
        hud(8, 0, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "%d", score_);
        hud(40 - int(std::strlen(buf)), 0, buf, PAL_GOLD);
        int n = left();
        if (mode_ == Mode::Pause) hudC(26, "PAUSED", PAL_WHITE);
        else if (n > 0) {
            std::snprintf(buf, sizeof buf, "LEFT %d", n);
            hudC(26, buf, PAL_WHITE);
        } else if (!hasKey_) hudC(26, "THE KEY", PAL_GOLD);
        else hudC(26, "THE DOOR", PAL_GREEN);
        if (playT_ < 4.f) hudC(27, "C SWINGS", PAL_DIM);
        else hud(40 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_DIM);
    } else if (mode_ == Mode::Victory) {
        std::snprintf(buf, sizeof buf, "%d", score_);
        hudC(26, "THE DOOR OPENS", PAL_GREEN);
        hudC(27, buf, PAL_GOLD);
    } else {
        hudC(26, "THE CORRIDOR HOLDS", PAL_RED);
        hudC(27, "START RETRIES", PAL_WHITE);
    }
    if (mode_ == Mode::Victory) sys_->setLight(200, 140, 40);
    else if (hurtFlash_ > 0) sys_->setLight(180, 30, 30);
    else if (mode_ == Mode::Play) sys_->setLight(50, 32, 24);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    paintHall(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(2, 2, 4));
    if (bot_) begin();
    else {
        resetLevel();
        mode_ = Mode::Title;
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update(DT);
    } else if (mode_ == Mode::Dead) {
        deadT_ -= DT;
        tickFx(DT);
        if (deadT_ <= 0) {
            if (lives_ <= 0) {
                mode_ = Mode::Over;
                over_ = true;
                won_ = false;
            } else revive();
        }
    } else if (mode_ == Mode::Over) {
        tickFx(DT);
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Victory) {
        tickFx(DT);
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            resetLevel();
            mode_ = Mode::Title;
        }
    } else tickFx(DT);

    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }
    fanStep(DT);
    if (shake_ > 0) shake_ = std::max(0.f, shake_ - DT);
    if (hurtFlash_ > 0) hurtFlash_ = std::max(0.f, hurtFlash_ - DT);
    draw();
}

}  // namespace gauntlet

#include "game/gig.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

namespace gig {
namespace {

constexpr float PLAYER_R = 0.36f;
constexpr float CAR_R = 0.86f;
constexpr float HOLE_R = 0.52f;
constexpr float CRATE_R = 0.88f;
constexpr float CAR_STOP = 0.18f;
constexpr float THROW_T = 0.40f;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

int fogOf(float zrel) {
    if (zrel < 2.5f) return 0;
    return std::clamp(int((zrel - 2.5f) * 0.5f), 0, 8);
}

}  // namespace

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.07f);
    beep_ = 0.07f;
}

void Game::pop(const std::string& text, float lat, float z, int pal) {
    pops_.push_back({text, lat, z, 0.9f, pal});
    if (pops_.size() > 8) pops_.erase(pops_.begin());
}

void Game::cam(float lat, float z, float& sx, float& sy) const {
    project(lat, z - pz_, sx, sy);
    sx += shakeX_;
    sy += shakeY_;
}

void Game::blit(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool feet, int fog, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 8 || s.y > gs::SCREEN_H + 8 || s.x + s.w < -8 || s.y + s.h < -8) {
        // Ground strips hang off the screen on purpose; still submit those.
        if (s.x + s.w < -40 || s.x > gs::SCREEN_W + 40 || s.y + s.h < -40 || s.y > gs::SCREEN_H + 40) return;
    }
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 12));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::hudText(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        int tile = art_.font[c - 32];
        if (!tile) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(tile, pal));
    }
}

float Game::lineWidth(const std::string& s, float h) const {
    float w = 0;
    for (char ch : s) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) {
            w += h * 0.35f;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        w += h * float(g.w) / float(std::max(1, g.h)) + 1.f;
    }
    return w;
}

void Game::lineText(const std::string& s, float x, float y, float h, int pal, int align) {
    float w = lineWidth(s, h);
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (char ch : s) {
        unsigned char c = static_cast<unsigned char>(ch);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) {
            x += h * 0.35f;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = h * float(g.w) / float(std::max(1, g.h));
        blit(g, x + gw * 0.5f, y, gw, h, pal);
        x += gw + 1.f;
    }
}

void Game::startShift() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    ammo_ = AMMO_MAX;
    delivered_ = 0;
    missed_ = 0;
    combo_ = 0;
    cents_ = 0;
    stars_ = 5.f;
    time_ = 0;
    px_ = 0.4f;
    pz_ = 0;
    vx_ = 0;
    spd_ = SPEED;
    stamina_ = 1.f;
    hop_ = hopY_ = 0;
    iframes_ = 0;
    shake_ = 0;
    banner_ = 2.6f;
    row_ = 0;
    nextZ_ = 10.f;
    note_ = "";
    houses_.clear();
    holes_.clear();
    crates_.clear();
    cars_.clear();
    trees_.clear();
    lamps_.clear();
    toss_.clear();
    pops_.clear();
    while (nextZ_ < 52.f) spawnRow();
}

void Game::spawnRow() {
    int row = row_++;
    float z = nextZ_;
    nextZ_ += SPACING;

    float j = (hash01(row * 5 + 1) - 0.5f) * 0.30f;
    House h;
    h.side = 1;
    h.deliver = true;
    h.pal = row % 4;
    h.lat = HOUSE_LAT + j;
    h.z = z + (hash01(row * 5 + 2) - 0.5f) * 0.36f;
    h.porchLat = h.lat - PORCH_IN;
    h.porchZ = h.z + 0.10f;
    h.mailLat = h.porchLat - 1.02f;
    h.mailZ = h.z - 0.70f;
    float want = h.porchLat - REACH;
    if (want > LANE_LIM - 0.08f) {
        float d = want - (LANE_LIM - 0.08f);
        h.lat -= d;
        h.porchLat -= d;
        h.mailLat -= d;
    }
    houses_.push_back(h);

    House left;
    left.side = -1;
    left.deliver = false;
    left.pal = (row + 2) % 4;
    left.lat = -(HOUSE_LAT + (hash01(row * 7 + 3) - 0.5f) * 0.30f);
    left.z = z + (hash01(row * 7 + 4) - 0.5f) * 0.46f;
    houses_.push_back(left);

    trees_.push_back({h.lat + 1.45f, h.z - 1.15f});
    trees_.push_back({left.lat - 1.35f, left.z - 0.9f});
    if (row % 2 == 0) lamps_.push_back({(row % 4 == 0) ? 4.25f : -4.25f, z + 0.2f});

    if (row >= 1 && row % 4 == 1) holes_.push_back({(row % 8 == 1) ? -1.15f : 0.40f, z + 5.5f, false});
    if (row >= 2 && row % 4 == 2) holes_.push_back({-0.25f, z + 3.4f, false});
    if (row >= 2 && row % 5 == 3) cars_.push_back({5.85f, z + 6.6f, false, false});
    if (row >= 1 && row % 3 == 1) crates_.push_back({2.10f, z + 4.1f, false});
}

void Game::cull() {
    auto gone = [&](float z) { return z < pz_ - 16.f; };
    houses_.erase(std::remove_if(houses_.begin(), houses_.end(), [&](const House& h) { return gone(h.z); }), houses_.end());
    holes_.erase(std::remove_if(holes_.begin(), holes_.end(), [&](const Hole& h) { return gone(h.z); }), holes_.end());
    crates_.erase(std::remove_if(crates_.begin(), crates_.end(), [&](const Crate& c) { return gone(c.z) || c.taken; }),
                  crates_.end());
    cars_.erase(std::remove_if(cars_.begin(), cars_.end(), [&](const Car& c) { return gone(c.z); }), cars_.end());
    trees_.erase(std::remove_if(trees_.begin(), trees_.end(), [&](const Tree& t) { return gone(t.z); }), trees_.end());
    lamps_.erase(std::remove_if(lamps_.begin(), lamps_.end(), [&](const Lamp& l) { return gone(l.z); }), lamps_.end());
}

Game::House* Game::nearest() {
    House* best = nullptr;
    float bestD = 1e9f;
    for (auto& h : houses_) {
        if (!h.deliver || h.used || h.missed) continue;
        float dz = h.porchZ - pz_;
        if (dz < -1.7f) continue;
        if (dz < bestD) {
            bestD = dz;
            best = &h;
        }
    }
    return best;
}

const Game::House* Game::nearest() const {
    const House* best = nullptr;
    float bestD = 1e9f;
    for (const auto& h : houses_) {
        if (!h.deliver || h.used || h.missed) continue;
        float dz = h.porchZ - pz_;
        if (dz < -1.7f) continue;
        if (dz < bestD) {
            bestD = dz;
            best = &h;
        }
    }
    return best;
}

void Game::botSteer() {
    float want = 1.4f;
    if (const House* h = nearest()) want = h->porchLat - REACH;
    float urgent = 5.f;
    auto dodge = [&](float lat, float z, float rad) {
        float dz = z - pz_;
        if (dz < -0.6f || dz > 3.6f) return;
        if (std::fabs(want - lat) > rad + 0.7f) return;
        if (std::fabs(dz) >= urgent) return;
        urgent = std::fabs(dz);
        float left = lat - (rad + 0.85f);
        float right = lat + (rad + 0.85f);
        want = (std::fabs(want - right) < std::fabs(want - left) && right <= LANE_LIM) ? right : left;
    };
    for (const auto& o : holes_)
        if (!o.hit) dodge(o.lat, o.z, HOLE_R);
    for (const auto& c : cars_)
        if (!c.hit) dodge(std::min(c.lat, CAR_STOP + 0.2f), c.z, CAR_R);
    // A backing car ends in the left lane. Squeeze right only when it is already there.
    for (const auto& c : cars_) {
        if (c.hit || c.lat > 2.4f) continue;
        float dz = c.z - pz_;
        if (dz < -0.8f || dz > 2.8f) continue;
        if (std::fabs(dz) < urgent) {
            urgent = std::fabs(dz);
            want = clampf(c.lat + CAR_R + PLAYER_R + 0.35f, -LANE_LIM, LANE_LIM);
        }
    }
    want = clampf(want, -LANE_LIM, LANE_LIM);
    float step = STEER_RATE * DT;
    float d = want - px_;
    if (std::fabs(d) <= step) px_ = want;
    else px_ += std::copysign(step, d);
}

bool Game::linedUp() const {
    const House* h = nearest();
    if (!h || ammo_ <= 0 || !toss_.empty()) return false;
    float dx = (px_ + REACH) - h->porchLat;
    float dz = (pz_ + LEAD) - h->porchZ;
    return dx * dx + dz * dz < 0.48f * 0.48f;
}

int Game::landAt(float lat, float z, bool take) {
    House* mail = nullptr;
    House* mat = nullptr;
    float bestM = 1e9f, bestP = 1e9f;
    for (auto& h : houses_) {
        if (!h.deliver || h.used) continue;
        float dm = std::hypot(lat - h.mailLat, z - h.mailZ);
        if (dm < MAIL_R && dm < bestM) {
            bestM = dm;
            mail = &h;
        }
        float dp = std::hypot(lat - h.porchLat, z - h.porchZ);
        if (dp < CATCH_R && dp < bestP) {
            bestP = dp;
            mat = &h;
        }
    }
    House* hit = mail ? mail : mat;
    int which = mail ? 2 : mat ? 1 : 0;
    if (take && hit) hit->used = true;
    return which;
}

void Game::score(int which, float lat, float z) {
    delivered_++;
    combo_++;
    int mult = 100 + (combo_ - 1) * 15;
    if (mult > 250) mult = 250;
    int base = which == 2 ? 1500 : 800;
    cents_ += base * mult / 100;
    pop(which == 2 ? "MAILBOX" : "TIP", lat, z, PAL_GOLD);
    blip(which == 2 ? 1040.f : 860.f);
    sys_->apu.tone(2, which == 2 ? 660.f : 520.f, 0.05f);
}

void Game::hurt(float dmg, int cost, const char* why, float lat, float z) {
    if (over_ || iframes_ > 0) return;
    iframes_ = 0.75f;
    stars_ = std::max(0.f, stars_ - dmg);
    combo_ = 0;
    cents_ = std::max(0, cents_ - cost);
    shake_ = 0.45f;
    note_ = why;
    pop(why, lat, z, PAL_RED);
    sys_->apu.noiseBurst(0.45f, 700.f, 0.22f);
    if (stars_ < 1.f) finish(false);
}

void Game::finish(bool win) {
    if (over_) return;
    over_ = true;
    won_ = win;
    mode_ = win ? Mode::Win : Mode::Lose;
    if (!win && stars_ >= 1.f) note_ = "time";
    titleStars_ = stars_;
    if (!sys_->headless && cents_ > best_) {
        best_ = cents_;
        sys_->saveBlob("best.txt", std::to_string(best_));
    }
    sys_->apu.tone(0, 0, 0);
}

void Game::update() {
    if (over_) return;
    time_ += DT;
    if (iframes_ > 0) iframes_ -= DT;
    if (banner_ > 0) banner_ -= DT;

    bool boost = false, brake = false, thr = false;
    if (bot_) {
        botSteer();
    } else {
        const gs::Pad& pad = sys_->pad;
        float steer = pad.axisX;
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        steer = clampf(steer, -1.f, 1.f);
        brake = pad.down(gs::BTN_DOWN);
        boost = pad.down(gs::BTN_UP) && !brake;
        thr = pad.pressed(gs::BTN_C);
        float target = steer * 4.8f;
        float slow = 1.f - 0.4f * std::fabs(steer);
        vx_ += (target - vx_) * std::min(1.f, DT * 9.f);
        px_ += vx_ * DT * (brake ? 0.45f : 1.f);
        spd_ = SPEED * slow;
        if (boost && stamina_ > 0.04f) spd_ *= 1.7f;
        if (brake) spd_ *= 0.18f;
    }
    if (bot_) {
        spd_ = SPEED;
        boost = false;
        brake = false;
    }
    if (boost && stamina_ > 0.04f) stamina_ = std::max(0.f, stamina_ - DT * 0.45f);
    else stamina_ = std::min(1.f, stamina_ + DT * 0.16f);
    px_ = clampf(px_, -LANE_LIM, LANE_LIM);
    pz_ += spd_ * DT;

    if (hop_ != 0.f || hopY_ > 0.f) {
        hopY_ += hop_ * DT;
        hop_ -= 24.f * DT;
        if (hopY_ <= 0.f) hop_ = hopY_ = 0.f;
    }

    while (nextZ_ < pz_ + 34.f) spawnRow();
    cull();

    if (bot_) thr = linedUp();
    if (thr) {
        if (ammo_ <= 0) {
            blip(110.f);
        } else if (toss_.empty() || !bot_) {
            ammo_--;
            Toss t;
            t.lat0 = px_;
            t.z0 = pz_;
            t.lat1 = px_ + REACH;
            t.z1 = pz_ + LEAD;
            t.t = THROW_T;
            toss_.push_back(t);
            blip(720.f);
        }
    }

    for (int i = int(toss_.size()) - 1; i >= 0; --i) {
        Toss& t = toss_[i];
        t.t -= DT;
        if (t.t > 0.f) continue;
        int which = landAt(t.lat1, t.z1, true);
        if (which) score(which, t.lat1, t.z1);
        toss_.erase(toss_.begin() + i);
    }

    for (auto& h : houses_) {
        if (!h.deliver || h.used || h.missed) continue;
        if (h.porchZ < pz_ - 0.40f) {
            h.missed = true;
            missed_++;
            combo_ = 0;
            cents_ = std::max(0, cents_ - 500);
            pop("MISSED", h.porchLat, h.porchZ, PAL_RED);
            blip(160.f);
        }
    }

    for (auto& c : cars_) {
        float dz = c.z - pz_;
        if (!c.go && dz < 9.2f && dz > -3.f) c.go = true;
        if (c.go && c.lat > CAR_STOP) c.lat = std::max(CAR_STOP, c.lat - 2.45f * DT);
        if (!c.hit && iframes_ <= 0.f && std::hypot(c.lat - px_, c.z - pz_) < CAR_R + PLAYER_R) {
            c.hit = true;
            hop_ = 3.2f;
            hurt(1.f, 1800, "CAR", c.lat, c.z);
        }
    }
    for (auto& o : holes_) {
        if (o.hit || iframes_ > 0.f) continue;
        if (std::hypot(o.lat - px_, o.z - pz_) < HOLE_R + PLAYER_R) {
            o.hit = true;
            hop_ = 5.4f;
            hurt(0.25f, 400, "POTHOLE", o.lat, o.z);
        }
    }
    for (auto& k : crates_) {
        if (k.taken) continue;
        if (std::hypot(k.lat - px_, k.z - pz_) < CRATE_R) {
            k.taken = true;
            ammo_ = AMMO_MAX;
            pop("RESTOCKED", k.lat, k.z, PAL_GREEN);
            blip(540.f);
        }
    }

    for (auto& p : pops_) p.t -= DT;
    pops_.erase(std::remove_if(pops_.begin(), pops_.end(), [](const Pop& p) { return p.t <= 0.f; }), pops_.end());

    if (over_) return;
    if (delivered_ >= TARGET && stars_ >= 1.f) finish(true);
    else if (stars_ < 1.f) finish(false);
    else if (time_ >= SHIFT_TIME) finish(false);
}

void Game::drawWorld() {
    // Earlier sprites sit on top of later ones. Banner, rider and the aim ring go first.
    if (mode_ == Mode::Pause) blit(art_.shade, 160, 108, 200, 48, PAL_WHITE);
    else if (mode_ == Mode::Win || mode_ == Mode::Lose) blit(art_.shade, 160, 104, 230, 78, PAL_WHITE);
    blit(art_.cup, 10, 6, 14, 16, PAL_PICK);

    float sx, sy;
    for (const Pop& p : pops_) {
        cam(p.lat, p.z, sx, sy);
        sy -= (0.9f - p.t) * 16.f;
        lineText(p.text, sx, sy, 14.f, p.pal, 0);
    }
    cam(px_, pz_, sx, sy);
    blit(art_.blob, sx, sy + 6.f, 26, 12, PAL_WHITE, false, 0, true);
    blit(art_.rider, sx, sy - hopY_ * 9.f, 22, 26, PAL_RIDER);
    for (const Toss& t : toss_) {
        float u = clampf(1.f - t.t / THROW_T, 0.f, 1.f);
        float lat = t.lat0 + (t.lat1 - t.lat0) * u;
        float z = t.z0 + (t.z1 - t.z0) * u;
        cam(lat, z, sx, sy);
        sy -= std::sin(u * 3.1415926f) * 22.f;
        blit(art_.cup, sx, sy, 16, 18, PAL_PICK);
    }
    float lx = px_ + REACH, lz = pz_ + LEAD;
    int aim = landAt(lx, lz, false);
    cam(lx, lz, sx, sy);
    float rp = 1.f + 0.06f * std::sin(t_ * 9.f);
    blit(art_.ring, sx, sy, art_.ring.w * rp, art_.ring.h * rp, aim ? PAL_GREEN : PAL_WHITE);

    struct Cmd {
        float key;
        int kind;
        int index;
        float pulse;
    };
    std::vector<Cmd> cmds;
    auto syOf = [&](float lat, float z) {
        float sx, sy;
        cam(lat, z, sx, sy);
        return sy;
    };
    float zStart = std::floor((pz_ - 12.f) / SEG) * SEG;
    for (float z = zStart; z < pz_ + 18.f; z += SEG) {
        float sx, sy;
        cam(0.f, z + SEG * 0.5f, sx, sy);
        cmds.push_back({sy - 4000.f, 0, int(std::lround(z * 10.f)), z});
    }
    for (int i = 0; i < int(houses_.size()); i++) cmds.push_back({syOf(houses_[i].lat, houses_[i].z), 1, i, 0});
    for (int i = 0; i < int(trees_.size()); i++) cmds.push_back({syOf(trees_[i].lat, trees_[i].z) + 2.f, 2, i, 0});
    for (int i = 0; i < int(lamps_.size()); i++) cmds.push_back({syOf(lamps_[i].lat, lamps_[i].z) + 3.f, 3, i, 0});
    for (int i = 0; i < int(holes_.size()); i++) cmds.push_back({syOf(holes_[i].lat, holes_[i].z) + 4.f, 4, i, 0});
    for (int i = 0; i < int(crates_.size()); i++)
        if (!crates_[i].taken) cmds.push_back({syOf(crates_[i].lat, crates_[i].z) + 6.f, 5, i, 0});
    for (int i = 0; i < int(cars_.size()); i++) cmds.push_back({syOf(cars_[i].lat, cars_[i].z) + 8.f, 6, i, 0});
    for (int i = 0; i < int(houses_.size()); i++) {
        if (!houses_[i].deliver) continue;
        float pulse = 1.f + 0.07f * std::sin(t_ * 6.f + houses_[i].z);
        cmds.push_back({syOf(houses_[i].porchLat, houses_[i].porchZ) + 24.f, 7, i, pulse});
        cmds.push_back({syOf(houses_[i].mailLat, houses_[i].mailZ) + 22.f, 8, i, 0});
    }

    std::sort(cmds.begin(), cmds.end(), [](const Cmd& a, const Cmd& b) { return a.key > b.key; });
    for (const Cmd& c : cmds) {
        float sx, sy;
        switch (c.kind) {
        case 0: {
            cam(0.f, c.pulse + SEG * 0.5f, sx, sy);
            blit(art_.ground, sx, sy, float(art_.ground.w), float(art_.ground.h), PAL_GROUND, false, 0);
            break;
        }
        case 1: {
            const House& h = houses_[c.index];
            cam(h.lat, h.z, sx, sy);
            blit(art_.house[h.side > 0 ? 0 : 1], sx, sy, float(art_.house[0].w), float(art_.house[0].h), PAL_H0 + h.pal,
                 false, fogOf(h.z - pz_));
            break;
        }
        case 2: {
            const Tree& tr = trees_[c.index];
            cam(tr.lat, tr.z, sx, sy);
            blit(art_.tree, sx, sy, float(art_.tree.w), float(art_.tree.h), PAL_NATURE, true, fogOf(tr.z - pz_));
            break;
        }
        case 3: {
            const Lamp& l = lamps_[c.index];
            cam(l.lat, l.z, sx, sy);
            blit(art_.lamp, sx, sy, float(art_.lamp.w), float(art_.lamp.h), PAL_NATURE, true, fogOf(l.z - pz_));
            break;
        }
        case 4: {
            const Hole& o = holes_[c.index];
            cam(o.lat, o.z, sx, sy);
            blit(art_.hole, sx, sy, float(art_.hole.w), float(art_.hole.h), PAL_NATURE, false, fogOf(o.z - pz_));
            break;
        }
        case 5: {
            const Crate& k = crates_[c.index];
            cam(k.lat, k.z, sx, sy);
            float g = 1.f + 0.12f * std::sin(t_ * 7.f + k.z);
            blit(art_.ring, sx, sy, art_.ring.w * g * 1.35f, art_.ring.h * g * 1.35f, PAL_GOLD, false, 0);
            blit(art_.crate, sx, sy - 4.f, float(art_.crate.w), float(art_.crate.h), PAL_PICK, false, 0);
            break;
        }
        case 6: {
            const Car& car = cars_[c.index];
            cam(car.lat, car.z, sx, sy);
            blit(art_.blob, sx, sy + 4.f, 36, 16, PAL_WHITE, false, 0, true);
            blit(art_.car, sx, sy, 52, 40, PAL_CAR, false, fogOf(car.z - pz_));
            break;
        }
        case 7: {
            const House& h = houses_[c.index];
            cam(h.porchLat, h.porchZ, sx, sy);
            if (h.used) blit(art_.matOff, sx, sy, float(art_.matOff.w), float(art_.matOff.h), PAL_GREEN, false, 0);
            else {
                blit(art_.mat, sx, sy, art_.mat.w * c.pulse, art_.mat.h * c.pulse, PAL_GREEN, false, 0);
            }
            break;
        }
        case 8: {
            const House& h = houses_[c.index];
            cam(h.mailLat, h.mailZ, sx, sy);
            blit(art_.mail, sx, sy, float(art_.mail.w), float(art_.mail.h), PAL_MAIL, true, fogOf(h.mailZ - pz_));
            break;
        }
        default:
            break;
        }
    }
}

void Game::drawPlayHud() {
    char buf[40];
    std::snprintf(buf, sizeof buf, "X%d", ammo_);
    hudText(2, 0, buf, PAL_WHITE);

    int left = std::max(0, int(std::ceil(SHIFT_TIME - time_)));
    std::snprintf(buf, sizeof buf, "%d:%02d", left / 60, left % 60);
    hudText(18, 0, buf, PAL_WHITE);

    int money = std::max(0, cents_);
    std::snprintf(buf, sizeof buf, "$%d.%02d", money / 100, money % 100);
    hudText(40 - int(std::strlen(buf)), 0, buf, PAL_GOLD);

    int filled = std::clamp(int(std::lround(stamina_ * 8.f)), 0, 8);
    for (int i = 0; i < 8; i++)
        sys_->vdp.HUD.set(i, 1, gs::entry(i < filled ? art_.tileBar : art_.tileBarDim, PAL_GREEN));

    if (combo_ >= 2) {
        std::snprintf(buf, sizeof buf, "%dX COMBO", combo_);
        hudText(40 - int(std::strlen(buf)), 1, buf, PAL_ORANGE);
    }

    std::snprintf(buf, sizeof buf, "%d/%d", delivered_, TARGET);
    hudText(28, 2, buf, PAL_WHITE);
    for (int i = 0; i < 5; i++) {
        bool on = stars_ > float(i) + 0.01f;
        sys_->vdp.HUD.set(34 + i, 2, gs::entry(on ? art_.tileStar : art_.tileStarDim, on ? PAL_GOLD : PAL_WHITE));
    }

    if (banner_ > 0.f && mode_ == Mode::Play) {
        hudText(11, 4, "SLEEPY CUL-DE-SAC", PAL_WHITE);
        hudText(13, 5, "TWELVE PORCHES", PAL_GREEN);
    }
    hudText(2, 26, "C THROW - LINE THE MARKER ON THE MAT", PAL_WHITE);
    hudText(1, 27, "ARROWS STEER   UP BOOST   DOWN BRAKE", PAL_GOLD);

    if (mode_ == Mode::Pause) {
        hudText(17, 12, "PAUSED", PAL_WHITE);
        hudText(14, 14, "ENTER RESUME", PAL_GOLD);
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        const char* head = mode_ == Mode::Win ? "SHIFT CLEAR" : (stars_ < 1.f ? "DEACTIVATED" : "OUT OF TIME");
        hudText((40 - int(std::strlen(head))) / 2, 10, head, mode_ == Mode::Win ? PAL_GREEN : PAL_RED);
        std::snprintf(buf, sizeof buf, "%d DELIVERIES", delivered_);
        hudText((40 - int(std::strlen(buf))) / 2, 12, buf, PAL_WHITE);
        std::snprintf(buf, sizeof buf, "STARS %.1f", double(stars_));
        hudText((40 - int(std::strlen(buf))) / 2, 14, buf, PAL_GOLD);
        std::snprintf(buf, sizeof buf, "$%d.%02d", money / 100, money % 100);
        hudText((40 - int(std::strlen(buf))) / 2, 16, buf, PAL_GOLD);
        hudText(17, 18, "ENTER", PAL_WHITE);
    }
}

void Game::drawTitle() {
    lineText("A DELIVERY DASH", 160, 42, 14, PAL_WHITE, 0);
    float h = 40.f;
    float wG = lineWidth("GIG", h);
    float wA = h * float(art_.letterA.w) / float(std::max(1, art_.letterA.h));
    float wB = lineWidth("BOY", h);
    float x = 160.f - (wG + wA + 4.f + wB) * 0.5f;
    lineText("GIG", x, 78, h, PAL_GREEN, -1);
    blit(art_.letterA, x + wG + wA * 0.5f, 80, wA, h, PAL_CYAN);
    lineText("BOY", x + wG + wA + 4.f, 78, h, PAL_GREEN, -1);

    char buf[40];
    std::snprintf(buf, sizeof buf, "CURRENT RATING: %.1f", double(titleStars_));
    int n = int(std::strlen(buf));
    int col = (40 - n) / 2;
    hudText(col, 13, buf, PAL_GOLD);
    sys_->vdp.HUD.set(col - 2, 13, gs::entry(art_.tileStar, PAL_GOLD));

    const char* labels[3] = {"START SHIFT", "UPGRADES", "LEADERBOARD"};
    const float ys[3] = {142.f, 172.f, 202.f};
    const int rows[3] = {17, 21, 25};
    for (int i = 0; i < 3; i++) {
        blit(art_.button, 160, ys[i], float(art_.button.w), float(art_.button.h), i == menu_ ? PAL_GREEN : PAL_WHITE);
        int ln = int(std::strlen(labels[i]));
        hudText((40 - ln) / 2, rows[i], labels[i], PAL_WHITE);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    bool street = mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Win || mode_ == Mode::Lose;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        float t = y / 223.f;
        if (!street) {
            float glow = std::sin(t * 3.14159f);
            int g = std::clamp(int(2 + 5 * glow), 0, 15);
            int b = std::clamp(int(7 - 4 * t), 0, 15);
            v.lineBackdrop[y] = gs::rgb4(1, g, b);
        } else if (t < 0.28f) {
            float u = t / 0.28f;
            v.lineBackdrop[y] = gs::rgb4(1, int(2 + 3 * u), int(7 - 2 * u));
        } else {
            float u = (t - 0.28f) / 0.72f;
            v.lineBackdrop[y] = gs::rgb4(int(2 + 7 * u), int(8 - 3 * u), int(4 - 2 * u));
        }
    }
    // The road chip's cross-section is horizontal, so this overhead street is sprites.
    if (street) {
        drawWorld();
        drawPlayHud();
    } else if (mode_ == Mode::Title) {
        drawTitle();
    } else if (mode_ == Mode::Upgrades) {
        hudText(16, 4, "UPGRADES", PAL_GREEN);
        hudText(6, 8, "THERMAL BAG", PAL_WHITE);
        hudText(26, 8, "LOCKED", PAL_GOLD);
        hudText(6, 10, "THROWING ARM", PAL_WHITE);
        hudText(26, 10, "LOCKED", PAL_GOLD);
        hudText(6, 12, "E-BIKE BATTERY", PAL_WHITE);
        hudText(26, 12, "LOCKED", PAL_GOLD);
        hudText(4, 16, "THE FIRST SHIFT PAYS FOR THESE", PAL_WHITE);
        hudText(16, 22, "ESC BACK", PAL_GOLD);
    } else if (mode_ == Mode::Board) {
        hudText(14, 4, "LEADERBOARD", PAL_GREEN);
        char buf[40];
        if (best_ <= 0) {
            hudText(12, 10, "NO SHIFTS LOGGED", PAL_WHITE);
        } else {
            std::snprintf(buf, sizeof buf, "BEST  $%d.%02d", best_ / 100, best_ % 100);
            hudText((40 - int(std::strlen(buf))) / 2, 10, buf, PAL_GOLD);
        }
        hudText(16, 22, "ESC BACK", PAL_GOLD);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(3, 5, 8));
    sys.apu.setMaster(0.8f);
    if (!sys.headless) {
        std::string s = sys.loadBlob("best.txt");
        if (!s.empty()) best_ = std::atoi(s.c_str());
    }
    if (bot_) startShift();
    else mode_ = Mode::Title;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    shake_ = std::max(0.f, shake_ - DT);
    shakeX_ = std::sin(t_ * 46.f) * shake_ * 6.f;
    shakeY_ = std::cos(t_ * 38.f) * shake_ * 4.f;
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys.apu.tone(1, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_UP) || pad.pressed(gs::BTN_DOWN)) {
            menu_ = (menu_ + (pad.pressed(gs::BTN_DOWN) ? 1 : 2)) % 3;
            blip(440.f);
        }
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) {
            blip(660.f);
            if (menu_ == 0) startShift();
            else if (menu_ == 1) mode_ = Mode::Upgrades;
            else mode_ = Mode::Board;
        } else if (pad.pressed(gs::BTN_MODE)) {
            sys.quit();
        }
        sys.apu.tone(0, 196.f, 0.022f);
    } else if (mode_ == Mode::Upgrades || mode_ == Mode::Board) {
        if (pad.pressed(gs::BTN_MODE) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))
            mode_ = Mode::Title;
        sys.apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
        if (mode_ == Mode::Play) sys.apu.tone(0, 42.f + spd_ * 6.f, 0.03f);
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) mode_ = Mode::Title;
        sys.apu.tone(0, 0, 0);
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) mode_ = Mode::Title;
        if (mode_ == Mode::Win) {
            int n = int(t_ * 5.f) % 4;
            const float f[4] = {523.f, 659.f, 784.f, 1046.f};
            sys.apu.tone(0, f[n], 0.04f);
        }
    }

    draw();
}

}  // namespace gig

#include "game/standard.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace standard {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 82.f;
constexpr float kSpan = 142.f;
constexpr float kZNear = 5.5f;
constexpr float kCam = 6.6f;
constexpr float kPpm = 44.f;
constexpr float kRoadHalf = 4.55f;
constexpr float kLine = 13.f;
constexpr float kFlagZ = 104.f;
constexpr float kFlagLat = 0.25f;
constexpr float kStartZ = 24.f;
constexpr float kMaxSpeed = 16.f;
constexpr float kAccel = 22.f;
constexpr float kBrake = 36.f;
constexpr float kDrag = 5.f;
constexpr float kSteer = 12.f;
constexpr float kLoop = 168.f;
constexpr float kLorrySpeed = 8.2f;
constexpr int kLives = 4;

float bend(float z) {
    if (z < 0.f) z = 0.f;
    float fade = z / (z + 36.f);
    return std::sin(z * 0.03f) * 3.6f * fade;
}

float camOf(float pz, int facing) { return pz - float(facing) * kCam; }

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory || mode_ == Mode::Fail) return 4;
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return 0;
    if (has_ && facing_ < 0) return 3;
    if (has_) return 2;
    return 1;
}

int Game::fogFor(float ahead) const {
    float t = kZNear / std::max(ahead, 0.8f);
    float f = std::clamp((0.22f - t) / 0.22f, 0.f, 1.f);
    return int(f * 13.f);
}

Game::Proj Game::project(float lat, float wz) const {
    Proj p;
    float ahead = (wz - camOf(pz_, facing_)) * float(facing_);
    if (!(ahead > kZNear * 0.42f)) return p;
    float t = kZNear / ahead;
    p.ppm = kPpm * t;
    p.y = kHorizon + t * kSpan;
    p.x = 160.f + (bend(wz) + lat - bend(camOf(pz_, facing_))) * p.ppm;
    p.ok = p.y > kHorizon - 16.f && p.y < gs::SCREEN_H + 60.f && p.x > -120.f && p.x < gs::SCREEN_W + 120.f;
    return p;
}

void Game::buildWorld() {
    props_.clear();
    for (int i = 0; i < 12; ++i) {
        Prop p;
        p.z = 18.f + float(i) * 12.5f;
        float side = (i & 1) ? 1.f : -1.f;
        p.lat = side * (kRoadHalf + 1.5f + float(i % 3) * 0.28f);
        p.kind = (i % 4 == 0) ? 1 : 0;
        props_.push_back(p);
    }
    lorries_.clear();
    const float zs[5] = {40.f, 40.f, 78.f, 122.f, 122.f};
    const float lats[5] = {-2.65f, 2.65f, 0.f, -2.65f, 2.65f};
    const int kinds[5] = {0, 0, 1, 0, 0};
    for (int i = 0; i < 5; ++i) {
        Lorry l;
        l.z = l.home = zs[i];
        l.lat = lats[i];
        l.speed = kLorrySpeed;
        l.kind = kinds[i];
        lorries_.push_back(l);
    }
}

void Game::layout() {
    pz_ = kStartZ;
    plat_ = 0;
    facing_ = 1;
    speed_ = 0;
    has_ = false;
    flagZ_ = kFlagZ;
    flagLat_ = kFlagLat;
    lives_ = kLives;
    lock_ = inv_ = stun_ = shake_ = skid_ = banner_ = hint_ = 0;
    wantTurn_ = false;
    wantT_ = 0;
    won_ = false;
    over_ = false;
    steerVis_ = 0;
    puffAcc_ = 0;
    grabT_ = 0;
    fanStep_ = -1;
    puffs_.clear();
    for (Lorry& l : lorries_) l.z = l.home;
}

void Game::begin() {
    layout();
    mode_ = Mode::Play;
    t_ = 0;
    hint_ = 2.4f;
    blip(true);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildWorld();
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.12f, 0.2f, 0.1f);
    if (bot_) begin();
    else {
        layout();
        mode_ = Mode::Title;
        t_ = 0;
    }
}

void Game::blip(bool high) {
    sys_->apu.tone(2, high ? 740.f : 170.f, 0.06f);
    blip_ = 0.08f;
}

void Game::fanfare(bool good) {
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = good;
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(2, 0.f, 0.f);
    }
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ >= 0.13f) {
            static const float good[] = {392.f, 523.f, 659.f, 784.f, 1046.f};
            static const float bad[] = {294.f, 220.f, 165.f};
            const float* n = fanGood_ ? good : bad;
            int count = fanGood_ ? 5 : 3;
            if (fanStep_ < count) sys_->apu.tone(0, n[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            sys_->apu.tone(1, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > count + 3) fanStep_ = -1;
        }
        return;
    }
    if (mode_ == Mode::Play && speed_ > 0.8f) {
        float f = 46.f + speed_ * 4.8f;
        sys_->apu.tone(0, f, 0.042f);
        sys_->apu.tone(1, f * 1.5f, 0.016f);
    } else if (mode_ == Mode::Title) {
        sys_->apu.tone(0, 82.f, 0.018f);
        sys_->apu.tone(1, 0.f, 0.f);
    } else {
        sys_->apu.tone(0, 0.f, 0.f);
        sys_->apu.tone(1, 0.f, 0.f);
    }
}

void Game::botPlan(float& steer, bool& gas, bool& brake, bool& turn) {
    gas = true;
    auto clearOf = [&](float lane) {
        float best = 90.f;
        for (const Lorry& l : lorries_) {
            if (std::fabs(l.lat - lane) > 1.2f) continue;
            float along = (l.z - pz_) * float(facing_);
            if (along > -3.6f && along < 80.f) best = std::min(best, std::max(0.f, along));
        }
        return best;
    };
    float here = clearOf(plat_);
    float look = 10.f + speed_ * 0.75f;
    float pref = has_ ? 0.f : flagLat_;
    if (here < look) {
        const float lanes[3] = {-2.65f, 0.f, 2.65f};
        float bestScore = -1.f;
        float bestLane = 0.f;
        for (float lane : lanes) {
            float score = clearOf(lane) - std::fabs(lane - plat_) * 0.2f;
            if (score > bestScore) {
                bestScore = score;
                bestLane = lane;
            }
        }
        pref = bestLane;
    }
    float err = pref - plat_;
    steer = std::clamp(err / 0.35f, -1.f, 1.f);

    bool needTurn = has_ ? facing_ > 0 : (flagZ_ - pz_) * float(facing_) < -2.4f;
    if (needTurn && here > 18.f) {
        gas = false;
        if (speed_ > 5.2f) brake = true;
        else turn = true;
    } else if (!has_ && here > 20.f && std::fabs(flagZ_ - pz_) < 9.f && std::fabs(flagLat_ - plat_) > 0.85f) {
        gas = false;
        if (speed_ > 6.f) brake = true;
    }
}

void Game::readControls(float& steer, bool& gas, bool& brake, bool& turn) {
    steer = 0;
    gas = false;
    brake = false;
    turn = false;
    if (bot_) {
        botPlan(steer, gas, brake, turn);
        return;
    }
    const gs::Pad& p = sys_->pad;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = p.axisX;
    steer = std::clamp(steer, -1.f, 1.f);
    gas = p.down(gs::BTN_UP) || p.down(gs::BTN_C) || p.down(gs::BTN_A) || p.accel > 0.22f;
    brake = p.down(gs::BTN_DOWN) || p.down(gs::BTN_B) || p.down(gs::BTN_X) || p.brake > 0.22f;
    turn = p.pressed(gs::BTN_TURBO) || p.pressed(gs::BTN_Z) || p.pressed(gs::BTN_Y);
}

void Game::grab() {
    if (has_ || lock_ > 0.f || stun_ > 0.f) return;
    if (std::fabs(pz_ - flagZ_) > 3.6f || std::fabs(plat_ - flagLat_) > 1.25f) return;
    has_ = true;
    banner_ = 1.5f;
    grabZ_ = flagZ_;
    grabLat_ = flagLat_;
    grabT_ = 0.4f;
    blip(true);
    sys_->setLight(170, 24, 24);
}

void Game::hit(float push) {
    if (inv_ > 0.f || mode_ != Mode::Play) return;
    --lives_;
    if (has_) {
        has_ = false;
        flagZ_ = pz_;
        flagLat_ = std::clamp(plat_, -2.1f, 2.1f);
        lock_ = 0.6f;
        banner_ = 0;
    }
    plat_ = std::clamp(plat_ + push, -4.15f, 4.15f);
    speed_ *= 0.15f;
    inv_ = 1.4f;
    stun_ = 0.38f;
    shake_ = 0.48f;
    sys_->rumble(0.7f, 0.35f, 160);
    sys_->apu.noiseBurst(0.22f, 1100.f, 9.f);
    blip(false);
    if (lives_ <= 0) lose();
}

void Game::collide() {
    if (inv_ > 0.f) return;
    for (const Lorry& l : lorries_) {
        if (std::fabs(l.z - pz_) >= 3.15f || std::fabs(l.lat - plat_) >= 1.0f) continue;
        float push = (plat_ >= l.lat) ? 1.85f : -1.85f;
        hit(push);
        return;
    }
}

void Game::win() {
    if (mode_ != Mode::Play || !has_) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    speed_ = 0;
    pz_ = kLine;
    fanfare(true);
    sys_->setLight(40, 170, 70);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    over_ = true;
    speed_ = 0;
    fanfare(false);
    sys_->setLight(140, 16, 16);
}

void Game::update() {
    float steer = 0;
    bool gas = false, brake = false, turn = false;
    if (stun_ <= 0.f) readControls(steer, gas, brake, turn);
    steerVis_ = steer;
    if (turn) {
        wantTurn_ = true;
        wantT_ = 1.7f;
    }
    if (wantTurn_) {
        wantT_ -= kDt;
        if (wantT_ <= 0.f) wantTurn_ = false;
        if (wantTurn_ && speed_ < 6.f && stun_ <= 0.f) {
            facing_ = -facing_;
            speed_ *= 0.4f;
            wantTurn_ = false;
            skid_ = 0.32f;
            blip(true);
        }
    }

    if (stun_ > 0.f) speed_ = std::max(0.f, speed_ - kBrake * kDt);
    else if (brake) speed_ = std::max(0.f, speed_ - kBrake * kDt);
    else if (gas) speed_ = std::min(kMaxSpeed, speed_ + kAccel * kDt);
    else speed_ = std::max(0.f, speed_ - kDrag * kDt);
    if (std::fabs(plat_) > 3.9f) speed_ = std::min(speed_, 7.f);

    plat_ += steer * kSteer * kDt;
    plat_ = std::clamp(plat_, -(kRoadHalf - 0.28f), kRoadHalf - 0.28f);
    pz_ += float(facing_) * speed_ * kDt;
    pz_ = std::clamp(pz_, 5.f, 148.f);

    for (Lorry& l : lorries_) {
        l.z -= l.speed * kDt;
        if (l.z < -8.f) l.z += kLoop;
    }

    auto decay = [](float& v) {
        if (v > 0.f) v = std::max(0.f, v - kDt);
    };
    decay(lock_);
    decay(inv_);
    decay(stun_);
    decay(shake_);
    decay(skid_);
    decay(banner_);
    decay(grabT_);
    decay(hint_);

    if (speed_ > 5.f) {
        puffAcc_ += kDt;
        if (puffAcc_ > 0.07f && puffs_.size() < 10) {
            puffAcc_ = 0;
            puffs_.push_back({pz_ - float(facing_) * 1.15f, plat_, 0.f});
        }
    }
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age > 0.42f; }), puffs_.end());

    grab();
    if (mode_ == Mode::Play) collide();
    if (mode_ == Mode::Play && has_ && pz_ <= kLine) win();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); ++i) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 33 || c > 126) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            width += 10.f * scale;
            continue;
        }
        width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false, false);
        x += gw;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet, bool shadow) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::road(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t fogC = shake_ > 0.22f ? gs::rgb4(12, 3, 2) : gs::rgb4(11, 8, 6);
    v.setFogColor(fogC);
    float origin = bend(camOf(pz_, facing_));
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        if (y <= int(kHorizon)) {
            v.road[y].on = false;
            float u = float(y) / kHorizon;
            int r = std::clamp(int(2.f + u * 10.f), 0, 15);
            int g = std::clamp(int(3.f + u * 3.5f), 0, 15);
            int b = std::clamp(int(9.f - u * 5.f), 0, 15);
            if (u > 0.65f) {
                float w = (u - 0.65f) / 0.35f;
                r = std::clamp(int(r + w * 4.f), 0, 15);
                g = std::clamp(int(g + w * 1.5f), 0, 15);
            }
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) - kHorizon) / kSpan;
        if (t < 0.004f) t = 0.004f;
        float ahead = kZNear / t;
        float wz = camOf(pz_, facing_) + float(facing_) * ahead;
        float ppm = kPpm * t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 160.f + (bend(wz) - origin) * ppm + shx;
        rd.hw = std::max(2.f, kRoadHalf * ppm);
        rd.v = wz * 32.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz / 4.f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((0.2f - t) / 0.2f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(std::min(16, int(fogT * 12.f) + (shake_ > 0.22f ? 3 : 0)));
        v.lineBackdrop[y] = gs::rgb4(2, 4, 2);
    }
    v.roadTime = int(t_ * 60.f);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    float shx = 0;
    if (shake_ > 0.f) shx = std::sin(t_ * 46.f) * 5.f * std::min(shake_, 1.f);
    road(shx);

    if (mode_ == Mode::Title) {
        text("S3 STANDARD", 160.f + shx, 30.f, 1.55f, PAL_ALERT);
        hudC(6, "TAKE THE FLAG OFF THE ROAD", PAL_GOLD);
        hudC(7, "AND BRING IT BACK", PAL_TEXT);
        hudC(26, "START RIDES", PAL_GOLD);
    } else if (mode_ == Mode::Victory) {
        text("BACK WITH THE LINES", 160.f + shx, 36.f, 1.15f, PAL_GOOD);
        hudC(26, "THE FLAG IS OFF THE ROAD", PAL_GOOD);
        hudC(27, "START RIDES AGAIN", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        text("STILL ON THE ROAD", 160.f + shx, 36.f, 1.2f, PAL_ALERT);
        hudC(26, "THE COLOUR DID NOT COME BACK", PAL_ALERT);
        hudC(27, "START RIDES AGAIN", PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(12, "HOLD", PAL_GOLD);
        hudC(27, "START RESUMES", PAL_TEXT);
    }

    struct Item {
        float ahead;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(48);
    auto aheadOf = [&](float wz) { return (wz - camOf(pz_, facing_)) * float(facing_); };

    if (!has_) items.push_back({aheadOf(flagZ_), 3, 0});
    if (grabT_ > 0.f) items.push_back({aheadOf(grabZ_), 8, 0});
    items.push_back({kCam, 7, 0});
    for (int i = 0; i < (int)puffs_.size(); ++i) items.push_back({aheadOf(puffs_[i].z) + 2.f, 0, i});
    for (int i = 0; i < (int)lorries_.size(); ++i) items.push_back({aheadOf(lorries_[i].z), 1, i});
    for (int i = 0; i < (int)props_.size(); ++i) items.push_back({aheadOf(props_[i].z), 2, i});
    items.push_back({aheadOf(kLine), 4, 0});
    items.push_back({aheadOf(kLine), 4, 1});
    items.push_back({aheadOf(kLine), 6, 0});
    items.push_back({aheadOf(7.f), 5, 0});
    items.push_back({800.f, 9, 0});
    items.push_back({800.f, 9, 1});
    items.push_back({800.f, 9, 2});
    items.push_back({900.f, 10, 0});

    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) {
        if (a.ahead != b.ahead) return a.ahead < b.ahead;
        return a.kind < b.kind;
    });

    int flutter = int(t_ * 8.f) & 1;
    float drawLat = plat_;
    if (mode_ == Mode::Title) drawLat = std::sin(t_ * 1.4f) * 0.35f;

    for (const Item& it : items) {
        if (it.kind == 7) {
            Proj p = project(drawLat, pz_);
            if (!p.ok) continue;
            bool blink = inv_ > 0.f && (int(t_ * 16.f) & 1);
            float bh = std::clamp(1.8f * p.ppm, 18.f, 150.f);
            float bob = speed_ > 1.f ? std::fabs(std::sin(t_ * (9.f + speed_ * 0.3f))) * 2.2f : 0.f;
            float x = p.x + shx + steerVis_ * 6.f;
            float y = p.y - bob;
            if (!blink) {
                if (has_) {
                    const gs::Mipped& fl = art_.flag[flutter];
                    spr(fl, x + bh * 0.38f, y - bh * 0.48f, bh * 1.05f, PAL_FLAG, false, 0, true, false);
                }
                spr(art_.bike, x, y, bh, PAL_BIKE, false, 0, true, false);
            }
            spr(art_.shadow, p.x + shx, p.y, bh * 0.22f, PAL_FX, false, 0, false, true);
            if (skid_ > 0.f) spr(art_.dust, p.x + shx, p.y, bh * 0.28f, PAL_FX, false, 0, false, false);
        } else if (it.kind == 1) {
            const Lorry& l = lorries_[it.id];
            Proj p = project(l.lat, l.z);
            if (!p.ok) continue;
            int fog = fogFor(it.ahead);
            bool front = facing_ > 0;
            const gs::Mipped& body = l.kind == 1 ? (front ? art_.carF : art_.carR) : (front ? art_.lorryF : art_.lorryR);
            float world = l.kind == 1 ? 2.15f : 3.25f;
            float h = std::clamp(world * p.ppm, 4.f, 150.f);
            spr(body, p.x + shx, p.y, h, PAL_LORRY, false, fog, true, false);
            spr(art_.shadow, p.x + shx, p.y, h * 0.16f, PAL_FX, false, 0, false, true);
        } else if (it.kind == 3) {
            Proj p = project(flagLat_, flagZ_);
            if (!p.ok) continue;
            float h = std::clamp(6.4f * p.ppm, 6.f, 160.f);
            spr(art_.flag[flutter], p.x + shx, p.y, h, PAL_FLAG, false, fogFor(it.ahead), true, false);
        } else if (it.kind == 8) {
            Proj p = project(grabLat_, grabZ_);
            if (!p.ok) continue;
            float h = std::clamp(2.4f * p.ppm, 8.f, 48.f);
            spr(art_.burst, p.x + shx, p.y - h * 0.4f, h, PAL_FX, false, 0, false, false);
        } else if (it.kind == 0) {
            const Puff& f = puffs_[it.id];
            Proj p = project(f.lat, f.z);
            if (!p.ok) continue;
            float h = std::clamp((1.1f + f.age * 2.f) * p.ppm, 3.f, 28.f);
            spr(art_.dust, p.x + shx, p.y, h, PAL_FX, false, fogFor(it.ahead), false, false);
        } else if (it.kind == 2) {
            const Prop& pr = props_[it.id];
            Proj p = project(pr.lat, pr.z);
            if (!p.ok) continue;
            float world = pr.kind == 1 ? 5.4f : 6.6f;
            float h = std::clamp(world * p.ppm, 4.f, 130.f);
            const gs::Mipped& m = pr.kind == 1 ? art_.tree : art_.poplar;
            spr(m, p.x + shx, p.y, h, PAL_TREE, pr.lat < 0, fogFor(it.ahead), true, false);
        } else if (it.kind == 4) {
            float lat = it.id == 0 ? -(kRoadHalf + 0.15f) : (kRoadHalf + 0.15f);
            Proj p = project(lat, kLine);
            if (!p.ok) continue;
            float h = std::clamp(3.5f * p.ppm, 5.f, 120.f);
            spr(art_.post, p.x + shx, p.y, h, PAL_CAMP, it.id == 1, fogFor(it.ahead), true, false);
        } else if (it.kind == 6) {
            Proj p = project(0.f, kLine);
            if (!p.ok) continue;
            float w = std::max(6.f, (kRoadHalf * 2.f - 0.4f) * p.ppm);
            float h = std::clamp(0.38f * p.ppm, 2.f, 12.f);
            sprBox(art_.tape, p.x + shx, p.y, w, h, PAL_CAMP, fogFor(it.ahead));
        } else if (it.kind == 5) {
            Proj p = project(-(kRoadHalf + 2.3f), 7.f);
            if (!p.ok) continue;
            float h = std::clamp(2.7f * p.ppm, 4.f, 90.f);
            spr(art_.tent, p.x + shx, p.y, h, PAL_CAMP, false, fogFor(it.ahead), true, false);
        } else if (it.kind == 9) {
            static const float cx[3] = {64.f, 170.f, 252.f};
            static const float cy[3] = {58.f, 68.f, 52.f};
            static const float ch[3] = {16.f, 12.f, 14.f};
            float drift = std::sin(t_ * 0.35f + it.id) * 8.f;
            spr(art_.cloud, cx[it.id] + drift + shx, cy[it.id], ch[it.id], PAL_FX, it.id == 1, 0, false, false);
        } else if (it.kind == 10) {
            spr(art_.sun, 292.f + shx, 18.f, 14.f, PAL_FX, false, 0, false, false);
        }
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        char lives[24];
        std::snprintf(lives, sizeof lives, "LIVES %d", lives_);
        hud(1, 0, lives, lives_ > 1 ? PAL_TEXT : PAL_ALERT);
        int dist = int(std::lround(has_ ? std::fabs(pz_ - kLine) : std::fabs(flagZ_ - pz_)));
        char far[24];
        std::snprintf(far, sizeof far, "%s %d", has_ ? "HOME" : "FLAG", dist);
        hud(40 - int(std::strlen(far)), 0, far, PAL_GOLD);
        const char* objective = "FLAG ON THE ROAD";
        if (has_ && facing_ > 0) objective = "TURN ABOUT";
        else if (has_) objective = "BRING IT BACK";
        if (mode_ == Mode::Play) hudC(26, objective, has_ ? PAL_GOLD : PAL_TEXT);
        if (banner_ > 0.f) hudC(12, "FLAG OFF THE ROAD", PAL_GOLD);
        else if (hint_ > 0.f) hudC(12, "RIDE FOR THE COLOUR", PAL_GOLD);
        else if (wantTurn_ && speed_ > 6.f) hudC(12, "SLOW DOWN TO TURN", PAL_ALERT);
        hudC(27, "ARROWS  C GAS  X BRAKE  SPACE TURN", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (mode_ != Mode::Pause) t_ += kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_MODE)) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (!bot_ && pad.pressed(gs::BTN_START)) {
        begin();
    }

    serviceAudio();
    draw();
}

}  // namespace standard

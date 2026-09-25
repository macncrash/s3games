#include "game/kart.h"

#include "game/track.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace kart {

namespace {

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    auto ch = [](int c0, int c1, float u) { return int(std::lround(c0 + (c1 - c0) * u)); };
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    return gs::rgb4(ch(ar, br, t), ch(ag, bg, t), ch(ab, bb, t));
}

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    buildArt(sys.vdp, art_);
    buildProps();
    if (bot_) begin();
    else mode_ = Mode::Title;
}

void Game::buildProps() {
    props_.clear();
    for (float s = 0; s < LAP; s += 34.f) {
        props_.push_back({s, HALF_W + 7.5f, Kind::Tree});
        if (int(s / 34.f) % 2 == 0) props_.push_back({s + 14.f, -(HALF_W + 8.f), Kind::Tree});
    }
    for (int i = 0; i < 3; i++) {
        props_.push_back({28.f + i * 40.f, HALF_W + 12.f, Kind::Stand});
        props_.push_back({STRAIGHT + TURN + 28.f + i * 40.f, HALF_W + 12.f, Kind::Stand});
    }
    props_.push_back({2.f, HALF_W + 5.5f, Kind::Flag});
    props_.push_back({2.f, -(HALF_W + 5.5f), Kind::Flag});
}

void Game::begin() {
    racers_.clear();
    struct Spec {
        float s, n, vmax, line, bias;
        int art;
        bool player;
    };
    const Spec spec[] = {
        {104.f, 1.2f, 46.6f, 0.34f, 1.1f, 1, false}, {92.f, -5.2f, 46.0f, 0.30f, -0.4f, 2, false},
        {92.f, 5.8f, 45.7f, 0.28f, 1.6f, 3, false},  {76.f, -5.0f, 45.2f, 0.22f, 0.3f, 4, false},
        {76.f, 6.0f, 44.6f, 0.18f, 2.0f, 5, false},  {8.f, 0.2f, MAX_V, 1.f, 0.f, 0, true},
    };
    me_ = 5;
    for (const Spec& s : spec) {
        Racer k;
        k.s = s.s;
        k.n = s.n;
        k.vmax = s.vmax;
        k.line = s.line;
        k.bias = s.bias;
        k.art = s.art;
        k.pal = PAL_PLAYER + s.art;
        k.player = s.player;
        racers_.push_back(k);
    }
    mode_ = Mode::Count;
    count_ = 0;
    over_ = false;
    won_ = false;
    camReady_ = false;
    place_ = 6;
    laps_ = 0;
    banner_ = 0;
    blip_ = 0;
    winAge_ = 0;
    shake_ = 0;
    gas_ = 0;
    hit_ = false;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    bool start = sys.pad.pressed(gs::BTN_START);
    bool confirm = start || sys.pad.pressed(gs::BTN_A);
    if (sys.pad.pressed(gs::BTN_MODE)) {
        if (mode_ == Mode::Title) sys.quit();
        else {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
        }
    }

    if (mode_ == Mode::Title) {
        if (confirm) begin();
        else {
            cine_ += 46.f * DT;
            if (cine_ > LAP * 6.f) cine_ -= LAP * 4.f;
            chase(cine_, racingLine(cine_) * 0.35f);
            present(false, false);
            audio();
            return;
        }
    }
    if (mode_ == Mode::Pause) {
        if (confirm && !bot_) mode_ = held_;
    } else if (mode_ == Mode::Race && start && !bot_) {
        held_ = Mode::Race;
        mode_ = Mode::Pause;
    } else if (mode_ == Mode::Count) {
        count_++;
        if (count_ >= 120) physics(DT);
        if (count_ >= 180 && mode_ == Mode::Count) mode_ = Mode::Race;
    } else if (mode_ == Mode::Race) {
        physics(DT);
    } else if (mode_ == Mode::Win || mode_ == Mode::Lose) {
        winAge_++;
        if (confirm) {
            begin();
            present(true, true);
            audio();
            return;
        }
    }

    bool giant = mode_ != Mode::Title;
    bool real = mode_ != Mode::Title;
    present(giant, real);
    audio();
}

void Game::readHuman(float& gas, float& brake, float& steer) const {
    const gs::Pad& p = sys_->pad;
    steer = 0;
    if (p.down(gs::BTN_LEFT)) steer -= 1.f;
    if (p.down(gs::BTN_RIGHT)) steer += 1.f;
    if (std::fabs(p.axisX) > 0.18f) steer = p.axisX;
    steer = std::clamp(steer, -1.f, 1.f);
    gas = (p.down(gs::BTN_UP) || p.down(gs::BTN_A) || p.down(gs::BTN_TURBO)) ? 1.f : 0.f;
    if (p.accel > gas) gas = p.accel;
    brake = (p.down(gs::BTN_DOWN) || p.down(gs::BTN_B)) ? 1.f : 0.f;
    if (p.brake > brake) brake = p.brake;
}

float Game::autoSteer(int ix) const {
    const Racer& k = racers_[size_t(ix)];
    float want = racingLine(k.s) * k.line + k.bias;
    for (int j = 0; j < int(racers_.size()); j++) {
        if (j == ix) continue;
        const Racer& o = racers_[size_t(j)];
        float ds = o.s - k.s;
        if (ds > -3.f && ds < 18.f && std::fabs(o.n - k.n) < 5.5f) {
            float side = (o.n - k.n) >= 0.f ? -1.f : 1.f;
            if (std::fabs(o.n - k.n) < 0.35f) side = -1.f;
            want += side * 7.f * (1.f - std::max(ds, 0.f) / 18.f);
        }
    }
    want = std::clamp(want, -(HALF_W - 2.f), HALF_W - 2.f);
    float err = want - k.n;
    float cent = k.speed * k.speed * curvature(k.s) * CENT_K;
    float desired = std::clamp(err * 5.5f, -LAT, LAT);
    return std::clamp((desired - cent) / LAT, -1.f, 1.f);
}

void Game::drive(Racer& k, float gas, float brake, float steer, float dt) {
    float cent = k.speed * k.speed * curvature(k.s) * CENT_K;
    k.n += (steer * LAT + cent) * dt;
    float cap = k.player ? MAX_V : k.vmax;
    bool grass = std::fabs(k.n) > HALF_W;
    bool wall = std::fabs(k.n) > HALF_W + BERM;
    if (grass) cap = std::min(cap, GRASS_V);
    if (brake > 0.05f) k.speed -= BRAKE * brake * dt;
    else if (gas > 0.05f) {
        if (k.speed < cap) k.speed += ACCEL * gas * dt;
    } else {
        k.speed -= COAST * dt;
    }
    if (k.speed > cap) k.speed -= 22.f * dt;
    if (k.speed < 0) k.speed = 0;
    if (wall) {
        k.n = std::clamp(k.n, -(HALF_W + BERM), HALF_W + BERM);
        if (!k.wall) k.speed *= 0.86f;
    }
    if (k.player && wall && !k.wall) {
        shake_ = 4.5f;
        hit_ = true;
    }
    k.wall = wall;
    k.steer = steer;
    float rate = 1.f;
    if (curvature(k.s) > 0.0001f) {
        float pathR = std::max(16.f, RADIUS + k.n);
        rate = RADIUS / pathR;
    }
    int prev = int(k.s / LAP);
    k.s += k.speed * rate * dt;
    if (k.player && int(k.s / LAP) > prev && int(k.s / LAP) < LAPS) {
        banner_ = 46;
        bannerN_ = int(k.s / LAP) + 1;
        blip_ = 10;
    }
}

void Game::bump() {
    for (int iter = 0; iter < 2; iter++) {
        for (int i = 0; i < int(racers_.size()); i++) {
            for (int j = i + 1; j < int(racers_.size()); j++) {
                float ds = racers_[size_t(j)].s - racers_[size_t(i)].s;
                float dn = racers_[size_t(j)].n - racers_[size_t(i)].n;
                if (std::fabs(ds) < 5.0f && std::fabs(dn) < 3.2f) {
                    float push = (3.2f - std::fabs(dn)) * 0.4f;
                    float dir = dn >= 0.f ? 1.f : -1.f;
                    racers_[size_t(i)].n -= dir * push;
                    racers_[size_t(j)].n += dir * push;
                    int rear = ds >= 0.f ? i : j;
                    racers_[size_t(rear)].speed *= 0.99f;
                }
            }
        }
    }
    for (Racer& k : racers_) k.n = std::clamp(k.n, -(HALF_W + BERM), HALF_W + BERM);
}

int Game::placeNow() const {
    int p = 1;
    float s = me().s;
    for (const Racer& k : racers_)
        if (!k.player && k.s > s) p++;
    return p;
}

void Game::finish(bool win) {
    won_ = win;
    over_ = true;
    place_ = placeNow();
    laps_ = std::min(LAPS, int(me().s / LAP));
    mode_ = win ? Mode::Win : Mode::Lose;
    winAge_ = 0;
}

void Game::physics(float dt) {
    if (racers_.empty() || mode_ == Mode::Win || mode_ == Mode::Lose) return;
    for (int i = 0; i < int(racers_.size()); i++) {
        Racer& k = racers_[size_t(i)];
        float gas = 1.f, brake = 0.f, steer = 0.f;
        if (k.player && !bot_) readHuman(gas, brake, steer);
        else steer = autoSteer(i);
        float cap = k.player ? MAX_V : k.vmax;
        if (!k.player && k.speed > cap) gas = 0.f;
        if (k.player && bot_ && k.speed > cap) gas = 0.f;
        gas_ = k.player ? gas : gas_;
        drive(k, gas, brake, steer, dt);
    }
    bump();
    place_ = placeNow();
    laps_ = std::min(LAPS, int(me().s / LAP));
    const float goal = LAP * LAPS;
    if (me().s >= goal) finish(placeNow() == 1);
    else {
        for (const Racer& k : racers_) {
            if (!k.player && k.s >= goal) {
                finish(false);
                break;
            }
        }
    }
}

void Game::chase(float s, float n) {
    Pose body = poseAt(s, n);
    float hdg = centerline(s).hdg;
    float fx = std::cos(hdg), fz = std::sin(hdg);
    camX_ = body.x - fx * CAM_BACK;
    camZ_ = body.z - fz * CAM_BACK;
    float h2 = centerline(s + 26.f).hdg;
    float want = hdg + wrapAngle(h2 - hdg) * 0.38f;
    if (!camReady_) {
        camHdg_ = want;
        camReady_ = true;
    } else {
        camHdg_ += wrapAngle(want - camHdg_) * 0.18f;
    }
    focus_ = s;
    shakeX_ = shake_ > 0 ? std::sin(float(sys_->frame) * 1.9f) * shake_ : 0.f;
    horizon_ = float(HORIZON) + (shake_ > 0 ? std::cos(float(sys_->frame) * 2.3f) * shake_ * 0.5f : 0.f);
    if (shake_ > 0) {
        shake_ *= 0.86f;
        if (shake_ < 0.08f) shake_ = 0;
    }
}

bool Game::project(float wx, float wz, float& sx, float& sy, float& z) const {
    float dx = wx - camX_, dz = wz - camZ_;
    float fx = std::cos(camHdg_), fz = std::sin(camHdg_);
    float rx = std::sin(camHdg_), rz = -std::cos(camHdg_);
    z = dx * fx + dz * fz;
    float lx = dx * rx + dz * rz;
    if (z < 0.85f) return false;
    float sc = FOCAL / z;
    sx = 160.f + sc * lx + shakeX_;
    sy = horizon_ + sc * CAM_H;
    return true;
}

void Game::sky() {
    gs::VDP& v = sys_->vdp;
    const uint16_t top = gs::rgb4(3, 5, 12);
    const uint16_t hor = gs::rgb4(11, 14, 15);
    const uint16_t grass = gs::rgb4(3, 8, 2);
    int hz = std::clamp(int(std::lround(horizon_)), 0, gs::SCREEN_H);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < hz) {
            float t = hz > 1 ? float(y) / float(hz) : 1.f;
            v.lineBackdrop[y] = lerpC(top, hor, t);
            int fog = y > hz - 20 ? (y - (hz - 20)) / 4 : 0;
            v.lineFog[y] = uint8_t(std::clamp(fog, 0, 8));
        } else {
            v.lineBackdrop[y] = grass;
            v.lineFog[y] = 0;
        }
    }
}

void Game::road() {
    gs::VDP& v = sys_->vdp;
    for (auto& r : v.road) r.on = false;
    struct Smp {
        float s, sx, sy, hw;
        bool ok;
    };
    Smp prev{};
    bool have = false;
    int maxy = gs::SCREEN_H;
    const float step = 4.f;
    for (int i = 0; i < 78; i++) {
        float s = focus_ - 28.f + i * step;
        Pose c = centerline(s);
        if (std::fabs(wrapAngle(c.hdg - camHdg_)) > 1.55f) break;
        float sx, sy, z;
        if (!project(c.x, c.z, sx, sy, z)) {
            have = false;
            continue;
        }
        float hw = FOCAL / z * HALF_W;
        float esx, esy, ez;
        Pose e = poseAt(s, HALF_W);
        if (project(e.x, e.z, esx, esy, ez)) hw = std::fabs(esx - sx);
        hw = std::max(hw, 0.6f);
        Smp cur{s, sx, sy, hw, true};
        if (have && cur.sy < prev.sy - 0.5f) {
            int y0 = std::max(0, int(std::ceil(cur.sy)));
            int y1 = std::min(maxy, int(std::ceil(prev.sy)));
            if (y1 > y0) {
                float inv = 1.f / (prev.sy - cur.sy);
                for (int y = y0; y < y1; y++) {
                    float tn = (prev.sy - (float(y) + 0.5f)) * inv;
                    gs::RoadLine& rl = v.road[y];
                    float ss = prev.s + (cur.s - prev.s) * tn;
                    float u = wrap(ss, LAP);
                    bool stripe = u < 6.f || u > LAP - 1.5f;
                    rl.on = true;
                    rl.cx = prev.sx + (cur.sx - prev.sx) * tn;
                    rl.hw = std::max(0.6f, prev.hw + (cur.hw - prev.hw) * tn);
                    rl.v = ss * 34.f;
                    rl.pal = uint8_t(stripe ? PAL_FINISH : PAL_ROAD);
                    rl.band = uint8_t(int(std::floor(ss / (stripe ? 3.f : 18.f))) & 1);
                    rl.style = 1;
                    rl.left = 0;
                    rl.right = 0;
                    int fog = 0;
                    if (y < int(horizon_) + 34) fog = int((horizon_ + 34.f - y) * 10.f / 34.f);
                    v.lineFog[y] = uint8_t(std::clamp(fog, 0, 11));
                }
                if (y0 < maxy) maxy = y0;
            }
        }
        prev = cur;
        have = true;
    }
    int bottom = -1;
    for (int y = 0; y < gs::SCREEN_H; y++)
        if (v.road[y].on) bottom = y;
    if (bottom >= 0) {
        for (int y = bottom + 1; y < gs::SCREEN_H; y++) {
            v.road[y] = v.road[bottom];
            v.road[y].hw = v.road[bottom].hw + float(y - bottom) * 4.f;
            v.lineFog[y] = 0;
        }
    }
}

void Game::blit(const gs::Image& img, float x, float y, float w, float h, int pal, int fog, bool shadow, bool flip) {
    if (img.w < 1 || img.h < 1 || w < 1.f || h < 1.f) return;
    if (x > gs::SCREEN_W + 8 || y > gs::SCREEN_H + 8 || x + w < -8 || y + h < -8) return;
    gs::Sprite s;
    s.img = img;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(std::min(w, 420.f)));
    s.h = int16_t(std::lround(std::min(h, 320.f)));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::queueKart(int artIx, int pal, float s, float n, float lean, bool giant) {
    if (giant) {
        const gs::Mipped& m = art_.kart[artIx];
        float sh = 78.f;
        float sw = sh * (float(m.w) / float(m.h));
        float x = 160.f - sw * 0.5f + lean * 14.f + shakeX_;
        float y = 176.f - sh;
        Bill b;
        b.z = 0.2f;
        b.x = x;
        b.y = y;
        b.w = sw;
        b.h = sh;
        b.img = m.lv[0];
        b.pal = pal;
        bills_.push_back(b);
        Bill shd;
        shd.z = 0.25f;
        shd.w = sw * 0.78f;
        shd.h = 13.f;
        shd.x = 160.f - shd.w * 0.5f + lean * 10.f + shakeX_;
        shd.y = y + sh - 10.f;
        shd.img = art_.shadow;
        shd.pal = pal;
        shd.shadow = true;
        bills_.push_back(shd);
        if (gas_ > 0.2f) {
            float ph = float(int(sys_->frame) % 10);
            Bill p;
            p.z = 0.45f;
            p.w = 8.f + ph * 0.4f;
            p.h = p.w;
            p.x = x + sw * 0.30f;
            p.y = y + sh - 16.f - ph;
            p.img = art_.puff;
            p.pal = PAL_PROP;
            p.fog = 4;
            bills_.push_back(p);
            p.x = x + sw * 0.58f;
            bills_.push_back(p);
        }
        return;
    }
    Pose body = poseAt(s, n);
    float sx, sy, z;
    if (!project(body.x, body.z, sx, sy, z)) return;
    if (z > 150.f) return;
    const gs::Mipped& m = art_.kart[artIx];
    float sh = FOCAL / z * SPR_H;
    if (sh < 3.f) return;
    float sw = sh * (float(m.w) / float(m.h));
    int fog = z > 36.f ? int((z - 36.f) / 9.f) : 0;
    fog = std::clamp(fog, 0, 13);
    Bill b;
    b.z = z;
    b.w = sw;
    b.h = sh;
    b.x = sx - sw * 0.5f;
    b.y = sy - sh;
    b.img = m.pick(sh);
    b.pal = pal;
    b.fog = fog;
    bills_.push_back(b);
    Bill shd;
    shd.z = z + 0.01f;
    shd.w = sw * 0.7f;
    shd.h = std::max(2.f, sh * 0.16f);
    shd.x = sx - shd.w * 0.5f;
    shd.y = sy - shd.h * 0.4f;
    shd.img = art_.shadow;
    shd.pal = pal;
    shd.shadow = true;
    shd.fog = fog;
    bills_.push_back(shd);
}

void Game::queueProps() {
    for (const Prop& p : props_) {
        Pose body = poseAt(p.s, p.n);
        float sx, sy, z;
        if (!project(body.x, body.z, sx, sy, z)) continue;
        if (z > 170.f) continue;
        const gs::Image* img = &art_.tree;
        float worldH = 5.4f;
        if (p.kind == Kind::Stand) {
            img = &art_.stand;
            worldH = 6.2f;
        } else if (p.kind == Kind::Flag) {
            img = &art_.flag;
            worldH = 4.4f;
        }
        float sh = FOCAL / z * worldH;
        if (sh < 3.f || img->h < 1) continue;
        float sw = sh * (float(img->w) / float(img->h));
        int fog = z > 30.f ? int((z - 30.f) / 11.f) : 0;
        Bill b;
        b.z = z;
        b.w = sw;
        b.h = sh;
        b.x = sx - sw * 0.5f;
        b.y = sy - sh;
        b.img = *img;
        b.pal = PAL_PROP;
        b.fog = std::clamp(fog, 0, 12);
        b.flip = p.n < 0 && p.kind == Kind::Flag;
        bills_.push_back(b);
    }
}

void Game::queueSky() {
    float pan = camHdg_ * 70.f;
    auto wrapX = [&](float x) {
        x = std::fmod(x - pan, 380.f);
        if (x < 0) x += 380.f;
        return x - 30.f;
    };
    Bill sun;
    sun.z = 8000.f;
    sun.x = wrapX(250.f);
    sun.y = 16.f;
    sun.w = 22.f;
    sun.h = 22.f;
    sun.img = art_.sun;
    sun.pal = PAL_PROP;
    bills_.push_back(sun);
    const float clouds[][2] = {{20, 28}, {120, 18}, {210, 34}, {300, 22}};
    for (auto& c : clouds) {
        Bill b;
        b.z = 7000.f;
        b.x = wrapX(c[0]);
        b.y = c[1];
        b.w = 56.f;
        b.h = 20.f;
        b.img = art_.cloud;
        b.pal = PAL_PROP;
        bills_.push_back(b);
    }
    for (int i = 0; i < 4; i++) {
        Bill b;
        b.z = 6000.f;
        b.x = wrapX(float(i) * 90.f);
        b.y = horizon_ - 34.f;
        b.w = 100.f;
        b.h = 24.f;
        b.img = art_.hill;
        b.pal = PAL_PROP;
        b.fog = 6;
        bills_.push_back(b);
    }
}

void Game::queueWorld(bool giant, bool real) {
    bills_.clear();
    queueSky();
    queueProps();
    if (real) {
        for (const Racer& k : racers_) {
            if (k.player && giant) queueKart(k.art, k.pal, k.s, k.n, k.steer, true);
            else queueKart(k.art, k.pal, k.s, k.n, 0.f, false);
        }
    } else {
        for (int i = 0; i < 6; i++) {
            float s = cine_ + 14.f + float(i) * 20.f;
            float n = racingLine(s) * 0.4f + float(i - 2) * 2.4f;
            n = std::clamp(n, -8.f, 8.f);
            queueKart(i, PAL_PLAYER + i, s, n, 0.f, false);
        }
    }
}

void Game::flush() {
    std::sort(bills_.begin(), bills_.end(), [](const Bill& a, const Bill& b) { return a.z < b.z; });
    for (const Bill& b : bills_) blit(b.img, b.x, b.y, b.w, b.h, b.pal, b.fog, b.shadow, b.flip);
    bills_.clear();
}

float Game::textWidth(const std::string& s, int scale) const {
    float w = 0;
    for (unsigned char ch : s) {
        int i = int(ch) - 32;
        if (i < 0 || i >= 96) {
            w += 5.f * scale;
            continue;
        }
        w += float(art_.gw[i] * scale);
    }
    return w;
}

void Game::hudText(const std::string& s, float x, float y, int pal, int scale) {
    float cx = x;
    for (unsigned char ch : s) {
        int i = int(ch) - 32;
        if (i < 0 || i >= 96) {
            cx += 5.f * scale;
            continue;
        }
        if (ch != ' ' && art_.glyph[i].w > 0)
            blit(art_.glyph[i], cx, y, float(art_.gw[i] * scale), float(art_.gh * scale), pal, 0, false, false);
        cx += float(art_.gw[i] * scale);
    }
}

void Game::hudCenter(const std::string& s, float y, int pal, int scale) {
    hudText(s, 160.f - textWidth(s, scale) * 0.5f, y, pal, scale);
}

void Game::stampAt(const Stamp& s, float x, float y, int pal) { blit(s.img, x, y, float(s.w), float(s.h), pal, 0, false, false); }

void Game::stampMid(const Stamp& s, float y, int pal) { stampAt(s, 160.f - s.w * 0.5f, y, pal); }

void Game::hud() {
    if (mode_ == Mode::Title) {
        stampMid(art_.title, 8, PAL_GOLD);
        stampMid(art_.sub, 42, PAL_TEXT);
        stampMid(art_.pitch, 66, PAL_GOLD);
        stampMid(art_.help, 176, PAL_TEXT);
        if ((sys_->frame / 30) % 2 == 0) stampMid(art_.press, 196, PAL_GOLD);
        return;
    }
    if (racers_.empty()) return;
    char buf[40];
    int done = std::min(LAPS, int(me().s / LAP));
    int show = done >= LAPS ? LAPS : done + 1;
    std::snprintf(buf, sizeof buf, "P%d  LAP %d/%d", place_, show, LAPS);
    hudText(buf, 8, 6, place_ == 1 ? PAL_GOLD : PAL_TEXT, 1);
    int spd = int(std::lround(me().speed * 2.8f));
    std::snprintf(buf, sizeof buf, "SPD %d", spd);
    hudText(buf, 8, 18, PAL_TEXT, 1);

    const float mapX = 232.f, mapY = 4.f;
    for (int i = int(racers_.size()) - 1; i >= 0; i--) {
        const Racer& k = racers_[size_t(i)];
        Pose p = poseAt(k.s, k.n);
        float x = mapX + MAP_CX + p.x * MAP_S;
        float y = mapY + MAP_CY - p.z * MAP_S;
        float d = k.player ? 7.f : 5.f;
        blit(art_.dot, x - d * 0.5f, y - d * 0.5f, d, d, k.pal, 0, false, false);
    }
    blit(art_.map, mapX, mapY, float(MAP_W), float(MAP_H), PAL_MAP, 0, false, false);

    if (mode_ == Mode::Count) {
        int lit = std::min(3, count_ / 40);
        for (int i = 0; i < 3; i++)
            blit(i < lit ? art_.lampOn : art_.lampOff, 136.f + i * 18.f, 36.f, 12, 12, PAL_LAMP, 0, false, false);
        if (count_ < 40) stampMid(art_.d3, 52, PAL_GOLD);
        else if (count_ < 80) stampMid(art_.d2, 52, PAL_GOLD);
        else if (count_ < 120) stampMid(art_.d1, 52, PAL_GOLD);
        else stampMid(art_.go, 52, PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        stampMid(art_.paused, 78, PAL_GOLD);
    } else if (mode_ == Mode::Win) {
        stampMid(art_.youwin, 28, PAL_GOLD);
        stampMid(art_.ahead, 64, PAL_TEXT);
        std::snprintf(buf, sizeof buf, "P%d   %d LAPS", place_, LAPS);
        hudCenter(buf, 96, PAL_TEXT, 1);
        if ((sys_->frame / 30) % 2 == 0) stampMid(art_.press, 196, PAL_TEXT);
    } else if (mode_ == Mode::Lose) {
        stampMid(art_.pack, 36, PAL_ALERT);
        std::snprintf(buf, sizeof buf, "P%d   %d/%d LAPS", place_, laps_, LAPS);
        hudCenter(buf, 80, PAL_TEXT, 1);
        if ((sys_->frame / 30) % 2 == 0) stampMid(art_.press, 196, PAL_TEXT);
    } else if (banner_ > 0 && mode_ == Mode::Race) {
        banner_--;
        std::snprintf(buf, sizeof buf, "LAP %d", bannerN_);
        hudCenter(buf, 46, PAL_GOLD, 2);
    } else if (!bot_ && mode_ == Mode::Race && curvature(me().s) > 0.001f && me().n > 3.f && me().steer > -0.35f) {
        hudCenter("TURN LEFT", 46, PAL_GOLD, 2);
    }
}

void Game::present(bool giant, bool real) {
    if (mode_ != Mode::Title) {
        if (!racers_.empty()) chase(me().s, me().n);
    }
    sys_->vdp.clearSprites();
    sky();
    road();
    queueWorld(giant, real);
    hud();
    flush();
}

void Game::audio() {
    gs::APU& a = sys_->apu;
    if (hit_) {
        a.noiseBurst(0.16f, 2400.f, 0.08f);
        sys_->rumble(0.55f, 0.25f, 90);
        hit_ = false;
    }
    if (sys_->pad.pressed(gs::BTN_C) && !bot_) a.noiseBurst(0.1f, 900.f, 0.12f);
    if (mode_ == Mode::Win) {
        static const float notes[] = {523.f, 659.f, 784.f, 1046.f, 784.f, 1046.f, 1318.f, 1568.f};
        int step = (winAge_ / 7) % 8;
        a.tone(0, notes[step], 0.06f);
        a.tone(1, notes[step] * 0.5f, 0.035f);
        a.tone(2, 0, 0);
        if (place_ == 1) sys_->setLight(30, 180, 50);
        return;
    }
    if (mode_ == Mode::Lose) {
        a.tone(0, 82.f, 0.04f);
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
        sys_->setLight(180, 30, 30);
        return;
    }
    if (mode_ != Mode::Race && mode_ != Mode::Count && mode_ != Mode::Pause) {
        a.tone(0, 0, 0);
        a.tone(1, 0, 0);
        a.tone(2, 0, 0);
        a.noise(0, 1000.f, false);
        return;
    }
    float spd = racers_.empty() ? 0.f : me().speed;
    float base = 46.f + spd * 5.2f;
    float vol = mode_ == Mode::Pause ? 0.02f : 0.045f;
    a.tone(0, base, vol);
    a.tone(1, base * 1.5f, vol * 0.55f);
    if (blip_ > 0) {
        blip_--;
        a.tone(2, 740.f, 0.05f);
    } else {
        a.tone(2, 0, 0);
    }
    a.noise(gas_ * 0.025f, 700.f + spd * 28.f, true);
    if (place_ == 1) sys_->setLight(40, 160, 50);
    else sys_->setLight(160, 90, 20);
}

}  // namespace kart

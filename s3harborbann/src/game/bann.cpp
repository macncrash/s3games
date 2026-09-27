#include "game/bann.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace hbann {
namespace {

constexpr float kHor = 72.f;
constexpr float kBoatY = 186.f;
constexpr float kNear = 28.f;
constexpr float kMouth = 680.f;
constexpr int kHull = 3;

float clampf(float v, float a, float b) { return std::max(a, std::min(b, v)); }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.hudEnabled = false;
    sys.apu.setMaster(0.7f);
    cut_[0] = {160.f, 0.4f, 0.034f, 0.72f};
    cut_[1] = {320.f, 2.1f, 0.029f, 0.68f};
    cut_[2] = {470.f, 4.2f, 0.031f, 0.74f};
    begin();
    mode_ = bot_ ? Mode::Run : Mode::Title;
}

void Game::begin() {
    over_ = false;
    won_ = false;
    carrying_ = false;
    reason_ = "THE BANNER IS STILL OUT";
    hull_ = kHull;
    inv_ = 0;
    clock_ = 0;
    fan_ = 0;
    z_ = 28.f;
    lat_ = 0.f;
    vz_ = 0.f;
    bannerZ_ = 560.f;
    shake_ = 0;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (!carrying_) return 1;
    if (z_ > 480.f) return 2;
    return 3;
}

float Game::cutterLat(int i) const {
    const Cutter& c = cut_[i];
    return std::sin(float(clock_) * c.w + c.phase) * c.amp;
}

int Game::gateAhead(bool outbound) const {
    int best = -1;
    float bestD = 1e9f;
    for (int i = 0; i < 3; ++i) {
        float dz = cut_[i].z - z_;
        bool hit = outbound ? (dz > 6.f && dz < 48.f && cut_[i].z < bannerZ_ - 16.f)
                            : (dz < -6.f && dz > -48.f && cut_[i].z > 40.f);
        if (!hit) continue;
        if (std::fabs(dz) < bestD) {
            bestD = std::fabs(dz);
            best = i;
        }
    }
    return best;
}

void Game::bot(float& steer, float& throttle) {
    bool out = !carrying_;
    int g = gateAhead(out);
    bool blocked = g >= 0 && std::fabs(cutterLat(g)) < 0.40f;
    float aim = 0.f;
    if (blocked) {
        float dz = cut_[g].z - z_;
        throttle = 0.f;
        if (out && dz < 22.f && vz_ > 0.05f) throttle = -1.f;
        if (!out && dz > -22.f && vz_ < -0.05f) throttle = 1.f;
    } else if (out) {
        float dz = bannerZ_ - z_;
        throttle = dz > 22.f ? 1.f : 0.32f;
    } else {
        throttle = z_ > 24.f ? -1.f : -0.28f;
    }
    float err = aim - lat_;
    steer = clampf(err * 4.f, -1.f, 1.f);
}

void Game::move(float steer, float throttle) {
    lat_ += steer * 0.03f;
    lat_ = clampf(lat_, -1.25f, 1.25f);
    vz_ += throttle * 0.055f;
    vz_ *= 0.9f;
    vz_ = clampf(vz_, -0.85f, 1.05f);
    z_ += vz_;
    if (z_ < 0.f) {
        z_ = 0.f;
        if (vz_ < 0.f) vz_ = 0.f;
    }
}

void Game::hazards() {
    if (std::fabs(lat_) > 1.08f) {
        lat_ = lat_ > 0 ? 0.92f : -0.92f;
        vz_ *= 0.4f;
        if (inv_ == 0) {
            hull_--;
            inv_ = 50;
            shake_ = 4.f;
            sys_->apu.noiseBurst(0.4f, 280.f, 0.16f);
            if (hull_ <= 0) lose("THE LAUNCH IS AGROUND");
        }
    }
    if (z_ > kMouth) {
        lose("YOU LEFT THE HARBOR");
        return;
    }
    if (inv_ > 0) inv_--;
    for (int i = 0; i < 3; ++i) {
        float dz = cut_[i].z - z_;
        float dl = cutterLat(i) - lat_;
        if (std::fabs(dz) < 11.f && std::fabs(dl) < 0.2f && inv_ == 0) {
            hull_--;
            inv_ = 46;
            shake_ = 5.f;
            vz_ *= -0.3f;
            lat_ += dl > 0 ? -0.12f : 0.12f;
            sys_->apu.noiseBurst(0.45f, 420.f, 0.18f);
            if (hull_ <= 0) lose("THE LAUNCH IS HOLED");
        }
    }
}

void Game::hoist() {
    if (carrying_) return;
    if (std::fabs(bannerZ_ - z_) < 12.f && std::fabs(lat_) < 0.22f) {
        carrying_ = true;
        sys_->apu.tone(1, 520.f, 0.16f);
        fan_ = 18;
    }
}

void Game::dock() {
    if (!carrying_) return;
    if (z_ < 10.f && std::fabs(lat_) < 0.28f && std::fabs(vz_) < 0.45f) win();
}

void Game::win() {
    mode_ = Mode::Won;
    over_ = true;
    won_ = true;
    reason_ = "THE BANNER IS HOME";
    fan_ = 70;
}

void Game::lose(const char* why) {
    if (over_) return;
    mode_ = Mode::Lost;
    over_ = true;
    won_ = false;
    reason_ = why;
    fan_ = 40;
}

void Game::tick() {
    clock_++;
    if (shake_ > 0.f) shake_ *= 0.86f;
    float steer = 0, throttle = 0;
    if (bot_) bot(steer, throttle);
    else {
        const gs::Pad& p = sys_->pad;
        if (p.down(gs::BTN_LEFT)) steer -= 1.f;
        if (p.down(gs::BTN_RIGHT)) steer += 1.f;
        if (p.down(gs::BTN_UP)) throttle += 1.f;
        if (p.down(gs::BTN_DOWN)) throttle -= 1.f;
        if (std::fabs(p.axisX) > 0.2f) steer = p.axisX;
        if (p.accel > 0.1f) throttle = p.accel;
        if (p.brake > 0.1f) throttle = -p.brake;
    }
    move(steer, throttle);
    hazards();
    if (!over_) hoist();
    if (!over_) dock();
    auto& a = sys_->apu;
    if (mode_ == Mode::Run) a.tone(0, 48.f + std::fabs(vz_) * 40.f, 0.03f);
    else a.tone(0, 0, 0);
    if (fan_ > 0) {
        fan_--;
        if (fan_ == 0) a.tone(1, 0, 0);
        else if (won_ && fan_ % 8 == 0) a.tone(1, 340.f + float(fan_), 0.1f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& p = sys.pad;
    if (mode_ == Mode::Title) {
        clock_++;
        if (bot_ || p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
            begin();
            mode_ = Mode::Run;
        }
    } else if (mode_ == Mode::Pause) {
        if (p.pressed(gs::BTN_START)) mode_ = Mode::Run;
    } else if (mode_ == Mode::Run) {
        if (!bot_ && p.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else tick();
    } else if (p.pressed(gs::BTN_START) || p.pressed(gs::BTN_A)) {
        begin();
        mode_ = Mode::Title;
    }
    draw();
}

float Game::halfAt(float y) const { return 12.f + (y - kHor) * 1.02f; }

float Game::dForY(float y) const {
    float k = (y - kHor) / (kBoatY - kHor);
    if (k < 0.04f) return 260.f;
    return kNear * (1.f - k) / k;
}

Game::Proj Game::project(float d, float lat) const {
    Proj p;
    if (d < -18.f || d > 300.f) return p;
    float k = kNear / (d + kNear);
    float y = kHor + (kBoatY - kHor) * k;
    p.y = y + oy_;
    p.x = 160.f + lat * halfAt(y) + ox_;
    p.s = clampf(0.22f + k, 0.16f, 1.6f);
    p.ok = p.y > -40.f && p.y < gs::SCREEN_H + 40.f;
    return p;
}

int Game::fogFor(float d) const {
    if (d > 160.f) return 11;
    if (d > 100.f) return 6;
    if (d > 55.f) return 2;
    return 0;
}

void Game::spr(const gs::Image& img, float cx, float cy, float dh, int pal, bool flip, int fog, bool shadow) {
    if (img.w == 0 || img.h == 0 || dh < 1.f) return;
    float sc = dh / float(img.h);
    gs::Sprite s;
    s.img = img;
    s.w = int16_t(std::max(1, int(std::lround(float(img.w) * sc))));
    s.h = int16_t(std::max(1, int(std::lround(dh))));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float dh, int pal, bool flip, int fog, bool shadow) {
    if (m.h <= 0) return;
    spr(m.pick(dh), cx, cy, dh, pal, flip, fog, shadow);
}

void Game::text(const std::string& s, float x, float y, int pal, float scale) {
    float h = float(art_.cellH) * scale;
    float a = float(art_.cellW) * scale;
    for (unsigned char ch : s) {
        if (ch != ' ' && ch < 128 && art_.glyph[ch].w) spr(art_.glyph[ch], x + 7.f * scale, y + h * 0.5f, h, pal);
        x += a;
    }
}

void Game::textC(const std::string& s, float y, int pal, float scale) {
    float w = float(s.size()) * float(art_.cellW) * scale;
    text(s, 160.f - w * 0.5f, y, pal, scale);
}

void Game::plate(float x, float y, float w, float h) {
    gs::Sprite s;
    s.img = art_.plate;
    s.x = int16_t(std::lround(x));
    s.y = int16_t(std::lround(y));
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.pal = PAL_PLATE;
    sys_->vdp.sprite(s);
}

void Game::skyRoad() {
    ox_ = std::sin(float(clock_) * 1.6f) * shake_;
    oy_ = std::cos(float(clock_) * 2.0f) * shake_ * 0.3f;
    float scroll = mode_ == Mode::Title ? float(clock_) * 0.3f : z_;
    auto& v = sys_->vdp;
    v.roadTime = int(sys_->frame);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        float t = clampf((float(y) + 8.f) / (kHor + 8.f), 0.f, 1.f);
        v.lineBackdrop[y] = gs::rgb4(int(1 + 6.f * t), int(3 + 5.f * t), int(8 - t));
        if (y < int(kHor)) {
            v.road[y].on = false;
            v.lineFog[y] = 0;
            continue;
        }
        float d = dForY(float(y));
        gs::RoadLine& ln = v.road[y];
        ln.on = true;
        ln.cx = 160.f + ox_;
        ln.hw = std::max(10.f, halfAt(float(y)));
        ln.v = (scroll + d) * 22.f;
        ln.pal = PAL_ROAD;
        ln.style = 2;
        ln.band = uint8_t((int(std::floor(scroll + d)) / 6) & 1);
        ln.left = gs::GROUND_LAND;
        ln.right = gs::GROUND_LAND;
        int fog = 0;
        if (d > 150.f) fog = 8;
        else if (d > 80.f) fog = 4;
        v.lineFog[y] = uint8_t(fog);
    }
}

void Game::world() {
    auto place = [&](float z, float lat, auto img, float h0, int pal, bool flip) {
        float d = z - z_;
        Proj p = project(d, lat);
        if (!p.ok) return;
        spr(img, p.x, p.y, h0 * p.s, pal, flip, fogFor(d));
    };
    for (int s = -1; s <= 1; s += 2) {
        place(6.f, float(s) * 1.28f, art_.pier, 70.f, PAL_QUAY, s > 0);
        place(40.f, float(s) * 1.32f, art_.pier, 60.f, PAL_QUAY, s > 0);
    }
    for (float z = 70.f; z < kMouth; z += 80.f) {
        for (int s = -1; s <= 1; s += 2) place(z, float(s) * 1.35f, art_.lamp, 28.f, PAL_QUAY, false);
    }
    for (float z = 50.f; z < bannerZ_; z += 90.f)
        for (int s = -1; s <= 1; s += 2) place(z, float(s) * 0.55f, art_.buoy, 16.f, PAL_WAKE, false);

    for (int i = 0; i < 3; ++i) {
        float d = cut_[i].z - z_;
        Proj p = project(d, cutterLat(i));
        if (!p.ok) continue;
        spr(art_.cutter, p.x, p.y, 48.f * p.s, PAL_CUTTER, cutterLat(i) > lat_, fogFor(d));
    }
    if (!carrying_) {
        float d = bannerZ_ - z_;
        Proj p = project(d, 0.f);
        if (p.ok) {
            spr(art_.raft, p.x, p.y, 30.f * p.s, PAL_QUAY, false, fogFor(d));
            spr(art_.banner, p.x, p.y - 16.f * p.s, 28.f * p.s, PAL_BANNER, false, fogFor(d));
        }
    }
    if (!(inv_ > 0 && (clock_ / 2) % 2 == 0)) {
        Proj b = project(0.f, lat_);
        if (b.ok) {
            spr(art_.boat, b.x, b.y + 3.f, 52.f, PAL_BOAT, false, 0, true);
            spr(art_.boat, b.x, b.y, 52.f, PAL_BOAT);
            if (carrying_) spr(art_.banner, b.x + 8.f, b.y - 22.f, 22.f, PAL_BANNER);
        }
    }
    for (int i = 1; i <= 3; ++i) {
        Proj w = project(-3.f - float(i) * 2.4f, lat_);
        if (w.ok) spr(art_.wake, w.x, w.y, 8.f + float(i) * 3.f, PAL_FX);
    }
    float drift = std::fmod(30.f + float(clock_) * 0.15f, 400.f) - 40.f;
    spr(art_.cloud, drift, 22, 22, PAL_SKY);
    spr(art_.cloud, std::fmod(160.f + float(clock_) * 0.08f, 440.f) - 20.f, 36, 16, PAL_SKY);
    spr(art_.sun, 270, 20, 26, PAL_SKY);
}

void Game::lettering() {
    if (mode_ == Mode::Title) {
        plate(16, 36, 288, 120);
        textC("S3 HARBOR BANN", 44, PAL_AMBER, 1);
        textC("YOU HAVE THE HARBOR", 68, PAL_TEXT, 1);
        textC("BRING THE BANNER BACK", 86, PAL_AMBER, 1);
        textC("ARROWS STEER AND DRIVE", 112, PAL_DIM, 1);
        textC("START TO CAST OFF", 130, PAL_TEXT, 1);
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        plate(24, 78, 272, 70);
        if (mode_ == Mode::Won) {
            textC("BANNER HOME", 88, PAL_AMBER, 1);
            textC("THE HARBOR HAS IT BACK", 110, PAL_TEXT, 1);
        } else {
            textC("HARBOR LOST", 88, PAL_RED, 1);
            textC(reason_, 110, PAL_TEXT, 1);
        }
    } else if (mode_ == Mode::Run || mode_ == Mode::Pause) {
        char buf[40];
        std::snprintf(buf, sizeof buf, "HULL %d", std::max(0, hull_));
        text(buf, 8, 4, hull_ <= 1 ? PAL_RED : PAL_TEXT, 1);
        text(carrying_ ? "RETURN" : "BANNER OUT", 150, 4, carrying_ ? PAL_AMBER : PAL_DIM, 1);
        if (mode_ == Mode::Pause) textC("PAUSED", 96, PAL_AMBER, 1);
    }
}

void Game::draw() {
    sys_->vdp.clearSprites();
    lettering();
    skyRoad();
    world();
}

}  // namespace hbann

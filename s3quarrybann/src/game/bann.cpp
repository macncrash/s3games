#include "game/bann.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace qbann {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHorizon = 58.f;
constexpr float kSpan = 156.f;
constexpr float kZNear = 6.6f;
constexpr float kPpm = 27.f;
constexpr float kBehind = 9.4f;
constexpr float kRoadHalf = 4.2f;
constexpr float kHome = 15.5f;
constexpr float kFace = 92.f;
constexpr float kBannerZ = 93.5f;
constexpr float kShift = 64.f;
constexpr float kMaxFwd = 16.f;
constexpr float kMaxBack = 14.f;

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Victory) return 3;
    if (mode_ == Mode::Fail) return 4;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return has_ ? 2 : 1;
    return 0;
}

float Game::camZ() const { return pz_ - kBehind; }

float Game::bend(float worldZ) const {
    float u = std::max(0.f, worldZ - 20.f);
    return std::sin(u * 0.045f) * 1.35f;
}

Game::Spot Game::project(float lat, float worldZ) const {
    Spot s;
    float wz = worldZ - camZ();
    if (!(wz > kZNear + 0.04f)) return s;
    float t = kZNear / wz;
    s.ppm = kPpm * t;
    s.y = kHorizon + t * kSpan;
    s.x = 160.f + (bend(worldZ) + lat) * s.ppm;
    s.ok = true;
    return s;
}

int Game::fogFor(float worldZ) const {
    float wz = worldZ - camZ();
    float t = kZNear / std::max(wz, 1.f);
    float fade = std::clamp((0.16f - t) / 0.16f, 0.f, 1.f);
    return int(fade * 12.f);
}

float Game::slideLat(const Slide& s) const { return std::sin(t_ * s.w + s.ph) * s.amp; }

void Game::buildProps() {
    props_.clear();
    auto add = [&](float z, float lat, float h, int kind) {
        Prop p;
        p.z = z;
        p.lat = lat;
        p.h = h;
        p.kind = kind;
        props_.push_back(p);
    };
    for (int i = 0; i < 7; ++i) {
        add(18.f + float(i) * 11.f, -(kRoadHalf + 3.1f), 7.4f + float(i % 3) * 0.5f, 0);
        add(24.f + float(i) * 10.5f, kRoadHalf + 3.3f, 6.6f + float((i + 2) % 3) * 0.55f, 0);
    }
    add(17.f, -(kRoadHalf + 1.6f), 4.4f, 1);
    add(20.f, -(kRoadHalf + 3.4f), 2.8f, 2);
    add(kHome, 0.f, 3.2f, 3);
    slides_[0] = {34.f, 2.15f, 0.85f, 0.3f};
    slides_[1] = {56.f, 2.2f, 1.05f, 1.8f};
    slides_[2] = {76.f, 2.05f, 0.95f, 2.6f};
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    has_ = false;
    lives_ = 3;
    pz_ = kHome + 1.f;
    lat_ = 0;
    vz_ = 0;
    t_ = 0;
    shift_ = kShift;
    plant_ = 0;
    hold_ = 0;
    inv_ = 0;
    edge_ = 0;
    shake_ = 0;
    commitZ_ = -1.f;
    reason_ = "THE BANNER IS NOT BACK";
    puffs_.clear();
}

void Game::begin() {
    bootTitle();
    mode_ = Mode::Play;
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.12f, 0.16f, 0.07f);
    if (bot_) begin();
    else bootTitle();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
    beep_ = 0.08f;
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.4f;
    shake_ = 1.f;
    blip(110.f);
    sys_->rumble(0.6f, 0.3f, 180);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    has_ = true;
    reason_ = "THE BANNER IS BACK";
    hold_ = 1.5f;
    vz_ = 0;
    blip(660.f);
    sys_->rumble(0.2f, 0.45f, 160);
}

void Game::botDrive(float& steer, float& throttle) {
    int dir = !has_ ? 1 : -1;
    bool settling = (!has_ && pz_ > kFace - 1.6f) || (has_ && pz_ < kHome + 1.1f);
    if (settling) dir = 0;

    if (commitZ_ >= 0.f) {
        float passed = dir >= 0 ? pz_ > commitZ_ + 2.4f : pz_ < commitZ_ - 2.4f;
        if (passed || dir == 0) commitZ_ = -1.f;
    }
    if (commitZ_ < 0.f && dir != 0) {
        float best = 1e9f;
        int pick = -1;
        for (int i = 0; i < 3; ++i) {
            float ahead = dir > 0 ? slides_[i].z - pz_ : pz_ - slides_[i].z;
            if (ahead > 0.4f && ahead < 13.f && ahead < best) {
                best = ahead;
                pick = i;
            }
        }
        if (pick >= 0 && best < 11.f) {
            float eta = best / (dir > 0 ? kMaxFwd : kMaxBack);
            float future = std::sin((t_ + eta) * slides_[pick].w + slides_[pick].ph) * slides_[pick].amp;
            commitLat_ = future >= 0.f ? -2.55f : 2.55f;
            commitZ_ = slides_[pick].z;
        }
    }

    float aim = commitZ_ >= 0.f ? commitLat_ : 0.f;
    if (settling) aim = 0.f;
    float d = aim - lat_;
    steer = std::clamp(d * 3.2f, -1.f, 1.f);

    if (!has_ && pz_ < kFace - 1.2f) throttle = 1.f;
    else if (has_ && pz_ > kHome + 0.8f) throttle = -1.f;
    else throttle = 0.f;
}

void Game::update() {
    float steer = 0.f, throttle = 0.f;
    if (bot_) {
        botDrive(steer, throttle);
    } else {
        const gs::Pad& pad = sys_->pad;
        if (pad.down(gs::BTN_LEFT)) steer -= 1.f;
        if (pad.down(gs::BTN_RIGHT)) steer += 1.f;
        if (std::fabs(pad.axisX) > 0.2f) steer = pad.axisX;
        if (pad.down(gs::BTN_UP)) throttle += 1.f;
        if (pad.down(gs::BTN_DOWN)) throttle -= 1.f;
        throttle += pad.accel;
        throttle -= pad.brake;
        throttle = std::clamp(throttle, -1.f, 1.f);
    }
    steer = std::clamp(steer, -1.f, 1.f);
    if (steer != 0.f) face_ = steer > 0.f ? 1 : -1;

    lat_ += steer * 7.2f * kDt;
    lat_ = std::clamp(lat_, -6.2f, 6.2f);

    float want = throttle > 0.15f ? kMaxFwd * throttle : throttle < -0.15f ? kMaxBack * throttle : 0.f;
    float rate = want == 0.f ? 22.f : 28.f;
    if (vz_ < want) vz_ = std::min(want, vz_ + rate * kDt);
    else vz_ = std::max(want, vz_ - rate * kDt);
    pz_ += vz_ * kDt;
    pz_ = std::clamp(pz_, 12.4f, 95.f);

    if (std::fabs(lat_) > kRoadHalf + 0.05f) edge_ += kDt;
    else edge_ = std::max(0.f, edge_ - kDt * 1.5f);
    if (edge_ > 0.42f) {
        lose("OFF THE HAUL");
        return;
    }

    if (inv_ > 0.f) inv_ -= kDt;
    if (inv_ <= 0.f) {
        for (const Slide& s : slides_) {
            if (std::fabs(pz_ - s.z) < 2.05f && std::fabs(lat_ - slideLat(s)) < 1.4f) {
                lives_ -= 1;
                inv_ = 1.15f;
                shake_ = 1.f;
                pz_ = std::clamp(pz_ - (vz_ >= 0.f ? 6.5f : -6.5f), 12.4f, 95.f);
                vz_ = 0;
                blip(90.f);
                sys_->rumble(0.8f, 0.4f, 140);
                sys_->apu.noiseBurst(0.35f, 1400.f, 0.18f);
                if (lives_ <= 0) {
                    lose("THE WATCH IS OVER");
                    return;
                }
                break;
            }
        }
    }

    if (!has_ && pz_ > kFace - 2.4f && std::fabs(lat_) < 1.9f) {
        has_ = true;
        blip(740.f);
    }
    if (has_ && pz_ < kHome + 1.15f && std::fabs(lat_) < 1.35f && std::fabs(vz_) < 2.4f) plant_ += kDt;
    else plant_ = std::max(0.f, plant_ - kDt);
    if (plant_ > 0.28f) {
        win();
        return;
    }

    shift_ -= kDt;
    if (shift_ <= 0.f) {
        lose("THE BANNER IS NOT BACK");
        return;
    }

    if (std::fabs(vz_) > 3.f) {
        puffT_ -= kDt;
        if (puffT_ <= 0.f) {
            puffT_ = 0.08f;
            Puff f;
            f.z = pz_ - 0.4f;
            f.lat = lat_;
            f.life = 0.38f;
            puffs_.push_back(f);
        }
    }
    for (Puff& f : puffs_) f.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& f) { return f.age > f.life; }),
                 puffs_.end());
}

void Game::serviceAudio() {
    if (beep_ > 0.f) beep_ -= kDt;
    else sys_->apu.tone(0, 0, 0);
    if (mode_ == Mode::Play && std::fabs(vz_) > 2.f) sys_->apu.tone(1, 46.f + std::fabs(vz_) * 2.4f, 0.035f);
    else sys_->apu.tone(1, 0, 0);
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool feet) {
    if (!(h > 1.5f) || m.h < 1 || m.w < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(feet ? cy - s.h : cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    float width = 0.f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (unsigned char c : s) {
        if (c < 33 || c > 126) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::layRoad(float shx) {
    gs::VDP& v = sys_->vdp;
    uint16_t sky = mode_ == Mode::Fail ? gs::rgb4(8, 3, 2) : gs::rgb4(10, 7, 4);
    v.setFogColor(sky);
    float cam = camZ();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            int r = int(5.f + u * 6.f);
            int g = int(3.f + u * 4.f);
            int b = int(2.f + u * 2.f);
            if (mode_ == Mode::Fail) r = std::min(15, r + 3);
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
            v.lineFog[y] = 0;
            rd.on = false;
            continue;
        }
        float row = float(y) - kHorizon;
        float t = std::max(row / kSpan, 0.004f);
        float wz = kZNear / t;
        float world = cam + wz;
        rd.on = true;
        rd.cx = 160.f + bend(world) * (kPpm * t) + shx;
        rd.hw = std::max(2.f, kRoadHalf * kPpm * t);
        rd.v = world * 18.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = gs::ROAD_ROCKY;
        rd.band = (int(std::floor(world * 0.17f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_DROP;
        float fogT = std::clamp((0.14f - t) / 0.14f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 11.f);
        v.lineBackdrop[y] = sky;
    }
    v.roadTime = int(t_ * 40.f);
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot s = project(pr.lat, pr.z);
    if (!s.ok) return;
    int fog = fogFor(pr.z);
    float h = std::clamp(pr.h * s.ppm, 6.f, 170.f);
    const gs::Mipped* m = &art_.cliff;
    int pal = PAL_ROCK;
    if (pr.kind == 1) {
        m = &art_.mill;
        pal = PAL_MILL;
    } else if (pr.kind == 2) {
        m = &art_.hopper;
        pal = PAL_MILL;
    } else if (pr.kind == 3) {
        m = &art_.post;
        pal = PAL_AMBER;
    }
    spr(*m, s.x + shx, s.y, h, pal, pr.lat > 0, fog, true);
}

void Game::drawBanner(float z, float lat, float shx, bool carried) {
    float flutter = std::sin(t_ * 6.f) * 0.18f;
    Spot s = project(lat + flutter, z);
    if (!s.ok) return;
    float h = std::clamp((carried ? 2.5f : 4.6f) * s.ppm, 10.f, 150.f);
    spr(art_.banner, s.x + shx, s.y - (carried ? h * 0.15f : 0.f), h, PAL_BANNER, false, fogFor(z), !carried);
}

void Game::drawSlide(int i, float shx) {
    const Slide& sld = slides_[i];
    float lat = slideLat(sld);
    Spot s = project(lat, sld.z);
    if (!s.ok) return;
    float h = std::clamp(2.3f * s.ppm, 8.f, 120.f);
    spr(art_.shadow, s.x + shx, s.y, h * 0.28f, PAL_DUST, false, 0, false);
    spr(art_.rock, s.x + shx, s.y, h, PAL_ROCK, lat > 0, fogFor(sld.z), true);
}

void Game::drawCrew(float shx) {
    Spot s = project(lat_, pz_);
    if (!s.ok) return;
    if (inv_ > 0.f && (int(t_ * 18.f) & 1)) return;
    float bob = std::sin(t_ * 14.f) * (std::fabs(vz_) > 2.f ? 2.f : 0.f);
    float h = std::clamp(2.35f * s.ppm, 16.f, 140.f);
    spr(art_.shadow, s.x + shx, s.y, h * 0.22f, PAL_DUST, false, 0, false);
    spr(art_.crew, s.x + shx, s.y - bob, h, PAL_CREW, face_ < 0, fogFor(pz_), true);
    if (has_) drawBanner(pz_ + 0.15f, lat_ + face_ * 0.35f, shx, true);
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 70.f) * 3.4f * std::min(shake_, 1.f);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt);
    layRoad(shx);

    if (mode_ == Mode::Title) text("QUARRY BANN", 160.f + shx, 24.f, 1.f, PAL_AMBER);
    else if (mode_ == Mode::Victory) text("BANNER BACK", 160.f + shx, 26.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text("NOT BACK", 160.f + shx, 26.f, 1.05f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 26.f, 1.1f, PAL_AMBER);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 0, i});
    for (int i = 0; i < 3; ++i) items.push_back({slides_[i].z, 1, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 2, i});
    if (!has_) items.push_back({kBannerZ, 3, 0});
    items.push_back({pz_, 4, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z > b.z; });
    for (auto it = items.rbegin(); it != items.rend(); ++it) {
        if (it->kind == 0) drawProp(props_[size_t(it->id)], shx);
        else if (it->kind == 1) drawSlide(it->id, shx);
        else if (it->kind == 2) {
            const Puff& f = puffs_[size_t(it->id)];
            Spot s = project(f.lat, f.z);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(s.ppm * (0.7f + u), 4.f, 28.f);
            spr(art_.dust, s.x + shx, s.y - u * 8.f, h, PAL_DUST, false, fogFor(f.z), false);
        } else if (it->kind == 3) drawBanner(kBannerZ, 0.f, shx, false);
        else drawCrew(shx);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "BRING THE BANNER BACK", PAL_AMBER);
        hudC(23, "ANYTHING ELSE IS A LOSS", PAL_ALERT);
        hudC(24, "UP DOWN THE HAUL", PAL_TEXT);
        hudC(25, "LEFT RIGHT  DODGE THE ROCK", PAL_TEXT);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE BANNER IS BACK", PAL_GOOD);
        hudC(24, "THE QUARRY KEEPS IT", PAL_AMBER);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "ANYTHING ELSE IS A LOSS", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else {
        std::snprintf(buf, sizeof buf, "CREW %d", lives_);
        hud(1, 0, buf, lives_ <= 1 ? PAL_ALERT : PAL_TEXT);
        int left = std::max(0, int(std::ceil(shift_)));
        std::snprintf(buf, sizeof buf, "SHIFT %d", left);
        hud(29, 0, buf, left <= 8 ? PAL_ALERT : PAL_TEXT);
        if (!has_) hudC(1, "TAKE THE BANNER", PAL_AMBER);
        else if (pz_ > kHome + 2.f) hudC(1, "BRING IT BACK", PAL_GOOD);
        else hudC(1, "PLANT IT IN THE YARD", PAL_GOOD);
        hud(1, 2, has_ ? "BANNER" : "EMPTY", has_ ? PAL_ALERT : PAL_TEXT);
        hudC(27, "UP DOWN    LEFT RIGHT", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    t_ += kDt;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || bot_) begin();
        else if (pad.pressed(gs::BTN_MODE)) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 150, 60);
    else if (mode_ == Mode::Fail) sys.setLight(180, 30, 24);
    else if (has_) sys.setLight(170, 40, 28);
    else sys.setLight(150, 100, 40);
}

}  // namespace qbann

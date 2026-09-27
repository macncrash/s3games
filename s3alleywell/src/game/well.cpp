#include "game/well.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace alleywell {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int HORIZON = 76;
constexpr float FOCAL = 268.0f;
constexpr float SPAWN_Z = 17.5f;
constexpr float HIT_Z = 2.15f;
constexpr float NEAR_Z = 2.35f;
constexpr float FAR_Z = 6.35f;
constexpr int WELL_HP = 4;

uint16_t lerpC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.0f, 1.0f);
    int ar = (a >> 8) & 15, ag = (a >> 4) & 15, ab = a & 15;
    int br = (b >> 8) & 15, bg = (b >> 4) & 15, bb = b & 15;
    auto ch = [&](int c0, int c1) { return int(std::lround(c0 + (c1 - c0) * t)); };
    return gs::rgb4(ch(ar, br), ch(ag, bg), ch(ab, bb));
}

float waveSpeed(int wave) { return wave == 0 ? 3.7f : wave == 1 ? 4.5f : 5.15f; }

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 6, 7));
    sys.apu.setMaster(0.8f);
    if (bot_) begin();
}

void Game::begin() {
    wave_ = 0;
    hp_ = WELL_HP;
    lane_ = 1;
    over_ = false;
    won_ = false;
    t_ = 0;
    shake_ = 0;
    cool_ = 0;
    swing_ = 0;
    armWave();
    mode_ = Mode::Play;
}

void Game::armWave() {
    spawns_.clear();
    threats_.clear();
    spawnAt_ = 0;
    waveT_ = 0;
    auto add = [&](float when, Kind k, int lane, int hp) { spawns_.push_back({when, k, lane, hp}); };
    if (wave_ == 0) {
        const int lanes[] = {1, 0, 2, 1, 0, 2};
        for (int i = 0; i < 6; i++) add(0.45f + i * 1.5f, Kind::Barrel, lanes[i], 1);
    } else if (wave_ == 1) {
        const int lanes[] = {0, 2, 1, 2, 0, 1, 2, 0};
        for (int i = 0; i < 8; i++) add(0.35f + i * 1.32f, i % 3 == 2 ? Kind::Cart : Kind::Barrel, lanes[i], 1);
    } else {
        const int lanes[] = {1, 0, 2, 1, 0, 2};
        for (int i = 0; i < 6; i++) {
            bool ram = i == 3;
            add(0.3f + i * 1.38f + (i > 3 ? 0.55f : 0), ram ? Kind::Ram : (i % 2 ? Kind::Cart : Kind::Barrel), lanes[i],
                ram ? 2 : 1);
        }
    }
}

const char* Game::kindName(Kind k) const {
    if (k == Kind::Cart) return "CART";
    if (k == Kind::Ram) return "RAM";
    return "BARREL";
}

void Game::botAct(int& lane, bool& shove) {
    const Threat* pick = nullptr;
    for (const Threat& th : threats_) {
        if (th.gone) continue;
        if (!pick || th.z < pick->z) pick = &th;
    }
    if (pick) lane = pick->lane;
    shove = false;
    if (!pick || cool_ > 0) return;
    if (pick->lane == lane_ && pick->z < FAR_Z && pick->z > NEAR_Z) shove = true;
}

void Game::update(float dt) {
    t_ += dt;
    waveT_ += dt;
    scroll_ += dt * (34.0f + wave_ * 8.0f);
    if (cool_ > 0) cool_ -= dt;
    if (swing_ > 0) swing_ -= dt;
    if (shake_ > 0) shake_ -= dt;

    const float speed = waveSpeed(wave_);
    while (spawnAt_ < int(spawns_.size()) && spawns_[spawnAt_].t <= waveT_) {
        const Spawn& s = spawns_[spawnAt_++];
        threats_.push_back({s.kind, s.lane, s.hp, SPAWN_Z, 0, false, -1});
        (void)kindName(s.kind);
    }

    if (swing_ > 0) {
        for (Threat& th : threats_) {
            if (th.gone || th.lane != lane_ || th.swung == swingId_) continue;
            if (th.z < FAR_Z && th.z > NEAR_Z - 0.15f) {
                th.hp -= 1;
                th.swung = swingId_;
                sys_->apu.tone(0, th.kind == Kind::Ram ? 140.0f : 220.0f, 0.08f);
                if (th.hp <= 0) {
                    th.gone = true;
                    th.slide = (th.lane == 0 ? -1.0f : th.lane == 2 ? 1.0f : (swingId_ & 1 ? 1.0f : -1.0f));
                    sys_->apu.noiseBurst(0.18f, 900.0f, 0.12f);
                }
            }
        }
    }

    for (Threat& th : threats_) {
        if (!th.gone) th.z -= speed * dt;
        else th.slide += (th.slide >= 0 ? 1.0f : -1.0f) * dt * 3.2f;
        if (!th.gone && th.z < HIT_Z) {
            th.gone = true;
            th.slide = 0;
            hp_ -= 1;
            shake_ = 0.35f;
            sys_->apu.noiseBurst(0.35f, 240.0f, 0.28f);
            sys_->apu.tone(1, 90.0f, 0.16f);
            if (hp_ <= 0) {
                hp_ = 0;
                mode_ = Mode::Fell;
                banner_ = 1.4f;
                over_ = true;
                won_ = false;
                return;
            }
        }
    }
    threats_.erase(std::remove_if(threats_.begin(), threats_.end(),
                                  [](const Threat& th) { return th.gone && std::fabs(th.slide) > 3.2f; }),
                   threats_.end());

    if (spawnAt_ >= int(spawns_.size())) {
        bool live = false;
        for (const Threat& th : threats_)
            if (!th.gone) live = true;
        if (!live) {
            if (wave_ >= 2) {
                mode_ = Mode::Stood;
                won_ = true;
                over_ = true;
                banner_ = 2.0f;
                sys_->apu.tone(2, 523.0f, 0.12f);
                return;
            }
            mode_ = Mode::Banner;
            banner_ = 1.35f;
            sys_->apu.tone(2, 392.0f, 0.1f);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || bot_) begin();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = held_;
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            held_ = mode_;
            mode_ = Mode::Pause;
        } else {
            int want = lane_;
            bool shove = false;
            if (bot_) botAct(want, shove);
            else {
                if (pad.pressed(gs::BTN_UP)) want = std::max(0, lane_ - 1);
                if (pad.pressed(gs::BTN_DOWN)) want = std::min(2, lane_ + 1);
                shove = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A);
            }
            lane_ = want;
            if (shove && cool_ <= 0) {
                cool_ = 0.34f;
                swing_ = 0.16f;
                swingId_++;
                sys.apu.tone(0, 330.0f, 0.05f);
            }
            update(DT);
        }
    } else if (mode_ == Mode::Banner) {
        banner_ -= DT;
        t_ += DT;
        scroll_ += DT * 12.0f;
        if (banner_ <= 0) {
            wave_++;
            armWave();
            mode_ = Mode::Play;
        }
    } else if (mode_ == Mode::Fell || mode_ == Mode::Stood) {
        banner_ -= DT;
        t_ += DT;
    }
    draw();
}

void Game::project(float x, float z, float& sx, float& sy, float& scale) const {
    float zz = std::max(0.85f, z);
    scale = FOCAL / zz;
    sx = 160.0f + x * scale * 0.62f;
    sy = float(HORIZON) + scale * 0.70f;
}

void Game::spr(const gs::Mipped& m, float cx, float foot, float h, int pal, bool flip, int fog) {
    if (h < 1.5f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(foot - s.h));
    if (s.x > gs::SCREEN_W + 40 || s.x + s.w < -40 || s.y > gs::SCREEN_H + 20 || s.y + s.h < -20) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    const float adv = 16.0f * scale;
    float w = float(s.size()) * adv;
    if (align == 0) x -= w * 0.5f;
    else if (align > 0) x -= w;
    for (size_t i = 0; i < s.size(); i++) {
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + i * adv + g.w * scale * 0.5f, y + g.h * scale, g.h * scale, pal, false, 0);
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

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (mode_ == Mode::Banner) return 2;
    if (mode_ == Mode::Fell) return 3;
    if (mode_ == Mode::Stood) return 4;
    return 1;
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.roadTime = int(t_ * 60);

    float shy = 0;
    if (shake_ > 0) shy = std::sin(t_ * 70.0f) * 3.0f * shake_ * 3.0f;

    const uint16_t skyTop = gs::rgb4(3, 4, 9);
    const uint16_t skyHor = gs::rgb4(12, 7, 5);
    for (int y = 0; y < gs::SCREEN_H; y++) {
        int yy = std::clamp(int(std::lround(y + shy)), 0, gs::SCREEN_H - 1);
        if (yy < HORIZON) {
            v.lineBackdrop[y] = lerpC(skyTop, skyHor, yy / float(HORIZON));
            v.lineFog[y] = 0;
            v.road[y].on = false;
            continue;
        }
        float row = float(yy - HORIZON) + 1.0f;
        gs::RoadLine& r = v.road[y];
        r.on = true;
        r.cx = 160;
        r.hw = 16.0f + row * 1.22f;
        r.v = scroll_ + 900.0f / row;
        r.pal = PAL_ROAD;
        r.band = (int(std::floor(r.v / 28.0f)) & 1) ? 1 : 0;
        r.style = gs::ROAD_ROCKY;
        r.left = r.right = gs::GROUND_SNOWWALL;
        v.lineFog[y] = uint8_t(std::clamp(int(11 - row / 9.0f), 0, 11));
        v.lineBackdrop[y] = skyHor;
    }

    auto fogOf = [](float z) { return std::clamp(int((z - 6.0f) * 0.7f), 0, 12); };
    auto laneX = [](int lane, float slide) {
        float x = (lane - 1) * 0.78f + slide;
        return x;
    };

    // Earlier sprites draw on top. The keeper stands in front of the well.
    if (mode_ == Mode::Play || mode_ == Mode::Pause || mode_ == Mode::Banner || mode_ == Mode::Stood) {
        float sx, sy, sc;
        project(laneX(lane_, 0), 2.05f, sx, sy, sc);
        const gs::Mipped& body = swing_ > 0 ? art_.shove : art_.keeper;
        spr(body, sx, sy + shy, 0.92f * sc, PAL_KEEPER, false, 0);
        if (swing_ > 0) spr(art_.dust, sx + (lane_ == 0 ? -18 : 18), sy + shy - 10, 0.28f * sc, PAL_FX, lane_ == 0, 0);
    }

    if (mode_ != Mode::Fell) {
        float sx, sy, sc;
        project(0, 1.55f, sx, sy, sc);
        float h = 1.28f * sc;
        spr(art_.well, sx, sy + shy, h, PAL_STONE, false, 0);
        if (hp_ < WELL_HP) spr(art_.wellCrack, sx, sy + shy, h, PAL_FX, false, 0);
    } else {
        float sx, sy, sc;
        project(0, 1.7f, sx, sy, sc);
        spr(art_.wellFell, sx, sy + shy, 0.72f * sc, PAL_STONE, false, 0);
    }

    std::vector<int> order;
    order.reserve(threats_.size());
    for (int i = 0; i < int(threats_.size()); i++) order.push_back(i);
    std::sort(order.begin(), order.end(), [&](int a, int b) { return threats_[a].z < threats_[b].z; });
    for (int i : order) {
        const Threat& th = threats_[i];
        if (th.gone && th.slide == 0 && th.z < HIT_Z) continue;
        float worldH = th.kind == Kind::Barrel ? 0.50f : th.kind == Kind::Cart ? 0.58f : 0.46f;
        const gs::Mipped& img = th.kind == Kind::Barrel ? art_.barrel : th.kind == Kind::Cart ? art_.cart : art_.ram;
        int pal = th.kind == Kind::Ram ? PAL_IRON : PAL_WOOD;
        float sx, sy, sc;
        project(laneX(th.lane, th.slide), th.z, sx, sy, sc);
        spr(img, sx, sy + shy, worldH * sc, pal, false, fogOf(th.z));
    }

    for (int side : {-1, 1}) {
        for (int i = 0; i < 7; i++) {
            float z = std::fmod(3.2f + i * 2.15f - std::fmod(scroll_ * 0.012f, 2.15f), 16.0f);
            if (z < 1.6f) z += 16.0f;
            float sx, sy, sc;
            project(side * 1.55f, z, sx, sy, sc);
            spr(art_.pier, sx, sy + shy, 1.55f * sc, PAL_BRICK, side < 0, fogOf(z));
            if (i % 2 == 0) {
                float wx, wy, wsc;
                project(side * 1.42f, z + 0.2f, wx, wy, wsc);
                spr(art_.window, wx, wy + shy - 0.85f * wsc, 0.42f * wsc, PAL_BRICK, false, fogOf(z));
            }
        }
    }

    if (mode_ == Mode::Title) {
        text("ALLEY WELL", 160, 58, 1.35f, PAL_STONE, 0);
        text("KEEP IT STANDING", 160, 92, 0.55f, PAL_HUD, 0);
        text("THREE WAVES", 160, 118, 0.5f, PAL_WOOD, 0);
        text("UP DOWN  LANE", 160, 156, 0.42f, PAL_HUD, 0);
        text("C  SHOVE", 160, 174, 0.42f, PAL_HUD, 0);
        if (int(t_ * 2) % 2 == 0) text("START", 160, 200, 0.5f, PAL_FX, 0);
        t_ += DT;
    } else if (mode_ == Mode::Pause) {
        text("PAUSE", 160, 96, 1.1f, PAL_HUD, 0);
    } else if (mode_ == Mode::Banner) {
        char buf[32];
        std::snprintf(buf, sizeof buf, "WAVE %d HELD", wave_ + 1);
        text(buf, 160, 86, 0.85f, PAL_KEEPER, 0);
        text("THE WELL STANDS", 160, 114, 0.48f, PAL_HUD, 0);
    } else if (mode_ == Mode::Fell) {
        text("THE WELL FELL", 160, 64, 0.85f, PAL_FX, 0);
    } else if (mode_ == Mode::Stood) {
        text("THE WELL STANDS", 160, 48, 0.72f, PAL_KEEPER, 0);
        text("THREE WAVES", 160, 78, 0.5f, PAL_HUD, 0);
    }

    if (mode_ != Mode::Title) {
        std::string blocks = "WELL ";
        for (int i = 0; i < WELL_HP; i++) blocks += i < hp_ ? '#' : '-';
        hud(1, 1, blocks, hp_ > 1 ? PAL_HUD : PAL_FX);
        char buf[24];
        std::snprintf(buf, sizeof buf, "WAVE %d/3", std::min(wave_ + 1, 3));
        hud(40 - int(std::strlen(buf)) - 1, 1, buf, PAL_HUD);
        if (mode_ == Mode::Play) hud(1, 26, "C SHOVE", PAL_HUD);
    }
}

}  // namespace alleywell

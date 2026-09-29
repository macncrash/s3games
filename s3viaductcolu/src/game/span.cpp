#include "game/span.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace vcol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr int kCrown = 1;
constexpr float kHorizon = 56.f;
constexpr float kSpan = 160.f;
constexpr float kZNear = 6.6f;
constexpr float kPpm = 30.f;
constexpr float kRoadHalf = 3.35f;
constexpr float kLine = 11.6f;
constexpr float kGap = 6.6f;
constexpr float kSpawn = 8.2f;
constexpr float kLeadZ = 68.f;
constexpr float kCruise = 4.85f;
constexpr float kWindowFar = 34.5f;
constexpr float kWindowNear = 16.2f;
constexpr float kWatch = 26.f;
constexpr float kLorryH = 2.85f;
constexpr float kPostLat[3] = {-2.05f, 0.f, 2.05f};

float curve(float z) {
    float u = std::max(0.f, z - 12.f);
    return std::sin(u * 0.022f) * 2.4f;
}

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

Game::Spot Game::project(float wx, float wz) const {
    Spot s;
    if (!(wz > kZNear + 0.05f)) return s;
    float t = kZNear / wz;
    s.ppm = kPpm * t;
    s.y = kHorizon + t * kSpan;
    s.x = 160.f + wx * s.ppm;
    s.ok = true;
    return s;
}

int Game::fogFor(float z) const {
    float t = kZNear / std::max(z, 1.f);
    float fade = std::clamp((0.16f - t) / 0.16f, 0.f, 1.f);
    return int(fade * 11.f);
}

float Game::leadZ() const {
    for (const Lorry& r : lorries_)
        if (r.slot == 0) return r.z;
    return 999.f;
}

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
    const float left = -(kRoadHalf + 1.15f);
    const float right = kRoadHalf + 1.15f;
    for (int i = 0; i < 7; ++i) {
        float z = 16.f + float(i) * 11.5f;
        add(z, left, 8.4f, 0);
        add(z + 2.2f, right, 8.4f, 0);
        add(z + 1.f, left - 0.15f, 2.2f, 1);
        add(z + 3.f, right + 0.1f, 2.2f, 1);
        add(z + 0.4f, left + 0.55f, 0.7f, 2);
        add(z + 2.6f, right - 0.55f, 0.7f, 2);
    }
}

void Game::lay(bool scenic) {
    lorries_.clear();
    puffs_.clear();
    for (int i = 0; i < kColumn; ++i) {
        Lorry r;
        r.slot = i;
        r.z = (scenic ? 42.f : kLeadZ) + float(i) * kSpawn;
        r.lat = 0.f;
        r.speed = scenic ? 0.f : kCruise;
        lorries_.push_back(r);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    locked_ = false;
    burned_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    walk_ = 1.f;
    post_ = kCrown;
    drop_ = 0.3f;
    hold_ = 0;
    settle_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    stopZ_ = 0;
    reason_ = "THE WATCH IS OVER";
    lay(true);
}

void Game::begin() {
    lay(false);
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    locked_ = false;
    burned_ = false;
    stopped_ = 0;
    through_ = 0;
    t_ = 0;
    walk_ = 1.f;
    post_ = kCrown;
    drop_ = 0;
    hold_ = 0;
    settle_ = 0;
    shake_ = 0;
    fanStep_ = -1;
    stopZ_ = 0;
    puffT_ = 0;
    reason_ = "THE WATCH IS OVER";
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.16f, 0.2f, 0.07f);
    if (bot_) begin();
    else bootTitle();
}

void Game::blip(float freq) {
    if (fanStep_ >= 0) return;
    sys_->apu.tone(0, freq, 0.05f);
    beep_ = 0.07f;
}

void Game::fanfare(bool good) {
    fanStep_ = 0;
    fanT_ = 0;
    fanGood_ = good;
}

void Game::lose(const char* why) {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Fail;
    won_ = false;
    reason_ = why;
    hold_ = 1.4f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.16f, 220.f, 0.2f);
    sys_->rumble(0.4f, 0.12f, 150);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.2f;
    fanfare(true);
    sys_->rumble(0.18f, 0.4f, 130);
}

const char* Game::hint() const {
    if (burned_) return "EARLY CHAIN";
    if (locked_) return "HOLD THE CHAIN";
    float z = leadZ();
    if (z > kWindowFar) return "WAIT FOR THE CROWN";
    if (post_ != kCrown) return "MAN THE CROWN";
    if (drop_ < 0.72f) return "DROP THE CHAIN";
    return "HOLD THE CHAIN";
}

void Game::update() {
    const gs::Pad& pad = sys_->pad;
    int want = post_;
    bool wantDrop = drop_ > 0.5f;
    if (bot_) {
        want = kCrown;
        float z = leadZ();
        wantDrop = !locked_ && !burned_ && z <= kWindowFar && z > kWindowNear + 1.4f &&
                   std::fabs(walk_ - float(want)) < 0.22f;
        if (locked_) wantDrop = true;
    } else {
        if (pad.pressed(gs::BTN_LEFT)) want = std::max(0, post_ - 1);
        if (pad.pressed(gs::BTN_RIGHT)) want = std::min(2, post_ + 1);
        wantDrop = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_A);
    }
    if (want != post_) {
        post_ = want;
        blip(300.f);
    }
    walk_ += std::clamp(float(post_) - walk_, -5.5f * kDt, 5.5f * kDt);
    float dropGoal = wantDrop ? 1.f : 0.f;
    drop_ += std::clamp(dropGoal - drop_, -3.4f * kDt, 3.4f * kDt);
    bool dropped = drop_ >= 0.72f && std::fabs(walk_ - float(post_)) < 0.2f;

    float zLead = leadZ();
    if (!locked_ && dropped && post_ == kCrown && zLead > kWindowFar + 0.35f) burned_ = true;
    if (burned_ && !dropped && zLead > kWindowFar) burned_ = false;

    if (!locked_ && !burned_ && dropped && post_ == kCrown && zLead <= kWindowFar && zLead > kWindowNear) {
        locked_ = true;
        stopZ_ = std::max(zLead, kLine + 4.f);
        blip(640.f);
        sys_->rumble(0.18f, 0.32f, 80);
    }

    int halted = 0;
    puffT_ += kDt;
    bool puff = puffT_ > 0.12f;
    if (puff) puffT_ = 0;
    for (Lorry& r : lorries_) {
        if (r.passed) continue;
        float haltZ = locked_ ? stopZ_ + float(r.slot) * kGap : -1.f;
        if (locked_ && r.z <= haltZ + 0.08f) {
            r.z = haltZ;
            r.speed = 0;
            r.stopped = true;
            r.lat = 0.f;
        } else {
            r.stopped = false;
            float cap = kCruise;
            if (locked_) cap = std::max(0.5f, (r.z - haltZ) * 1.2f);
            r.speed += std::clamp(cap - r.speed, -6.5f * kDt, 3.6f * kDt);
            r.z -= r.speed * kDt;
            r.lat = curve(r.z) * 0.04f;
            if (r.z < kLine) {
                r.passed = true;
                r.speed = 0;
                ++through_;
                lose("THE COLUMN PASSED");
            }
        }
        if (r.stopped) ++halted;
        if (puff && r.speed > 0.7f) {
            Puff f;
            f.z = r.z + 1.2f;
            f.lat = r.lat + ((r.slot & 1) ? 0.55f : -0.55f);
            f.life = 0.38f;
            puffs_.push_back(f);
        }
    }
    stopped_ = halted;

    if (mode_ == Mode::Play && !locked_ && dropped && post_ != kCrown && zLead <= kWindowFar && zLead > kWindowNear)
        lose("WRONG PIER");
    if (mode_ == Mode::Play && !locked_ && zLead <= kWindowNear) lose("THE COLUMN PASSED");
    if (mode_ == Mode::Play && burned_ && zLead <= kWindowFar) lose("EARLY CHAIN");
    if (mode_ == Mode::Play && locked_ && stopped_ == kColumn) {
        settle_ += kDt;
        if (settle_ > 0.65f && dropped) win();
        else if (settle_ > 0.65f && !dropped) {
            locked_ = false;
            settle_ = 0;
            lose("THE COLUMN ROLLED");
        }
    } else {
        settle_ = 0;
    }
    if (mode_ == Mode::Play && t_ >= kWatch) lose("THE WATCH IS OVER");
    t_ += kDt;
}

void Game::tickPuffs() {
    for (auto it = puffs_.begin(); it != puffs_.end();) {
        it->age += kDt;
        if (it->age >= it->life) it = puffs_.erase(it);
        else ++it;
    }
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f && fanStep_ < 0) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (fanStep_ >= 0) {
        fanT_ += kDt;
        if (fanT_ > 0.12f) {
            static const float good[] = {349.f, 440.f, 523.f, 698.f};
            static const float bad[] = {220.f, 174.f, 130.f};
            const float* notes = fanGood_ ? good : bad;
            int n = fanGood_ ? 4 : 3;
            if (fanStep_ < n) sys_->apu.tone(0, notes[fanStep_], 0.07f);
            else sys_->apu.tone(0, 0.f, 0.f);
            ++fanStep_;
            fanT_ = 0.f;
            if (fanStep_ > n + 2) fanStep_ = -1;
        }
    }
    bool rolling = false;
    if (mode_ == Mode::Play) {
        for (const Lorry& r : lorries_)
            if (!r.passed && r.speed > 0.45f) rolling = true;
    }
    if (rolling) sys_->apu.tone(2, 38.f, 0.028f);
    else sys_->apu.tone(2, 0.f, 0.f);
    if (drop_ > 0.15f && drop_ < 0.95f && mode_ == Mode::Play) sys_->apu.tone(1, 110.f, 0.03f);
    else sys_->apu.tone(1, 0.f, 0.f);
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

void Game::sprBox(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, int fog) {
    if (!(h > 1.f) || !(w > 1.f) || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::clamp(int(std::lround(cx - s.w * 0.5f)), -2000, 2000));
    s.y = int16_t(std::clamp(int(std::lround(cy - s.h * 0.5f)), -2000, 2000));
    s.img = m.pick(std::max(w, h));
    s.pal = uint8_t(pal);
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
    uint16_t mist = mode_ == Mode::Fail ? gs::rgb4(8, 3, 3) : gs::rgb4(8, 9, 12);
    v.setFogColor(mist);
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& rd = v.road[y];
        if (y < int(kHorizon)) {
            float u = float(y) / kHorizon;
            int r = int(5.f + u * 4.f);
            int g = int(6.f + u * 4.f);
            int b = int(9.f + u * 4.f);
            if (mode_ == Mode::Fail) {
                r = std::min(15, r + 4);
                b = std::max(2, b - 3);
            }
            v.lineBackdrop[y] = gs::rgb4(std::clamp(r, 0, 15), std::clamp(g, 0, 15), std::clamp(b, 0, 15));
            v.lineFog[y] = 0;
            rd.on = false;
            continue;
        }
        float row = float(y) - kHorizon;
        float t = std::max(row / kSpan, 0.004f);
        float wz = kZNear / t;
        float gorge = std::clamp((float(y) - kHorizon) / 150.f, 0.f, 1.f);
        int gr = int(2.f + (1.f - gorge) * 2.f);
        int gg = int(4.f + (1.f - gorge) * 3.f);
        int gb = int(6.f + (1.f - gorge) * 4.f);
        v.lineBackdrop[y] = gs::rgb4(gr, gg, gb);
        rd.on = true;
        rd.cx = 160.f + curve(wz) * (kPpm * t) + shx;
        rd.hw = std::max(2.f, kRoadHalf * kPpm * t);
        rd.v = wz * 24.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.18f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_DROP;
        rd.right = gs::GROUND_DROP;
        float fogT = std::clamp((0.13f - t) / 0.13f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 10.f);
    }
}

void Game::drawWatch(float shx) {
    float lat = kPostLat[std::clamp(post_, 0, 2)];
    float standZ = 10.4f;
    float wx = curve(standZ) + lat;
    Spot feet = project(wx, standZ);
    if (!feet.ok) return;
    int fog = fogFor(standZ);
    float h = std::clamp(2.05f * feet.ppm, 16.f, 110.f);
    float x = feet.x + shx;
    spr(art_.shadow, x, feet.y, h * 0.2f, PAL_SMOKE, false, 0, false);
    spr(art_.sentry, x, feet.y, h, PAL_COAT, post_ > kCrown, fog, true);
    if (drop_ > 0.08f) {
        float bh = h * (0.28f + 0.4f * drop_);
        spr(art_.chain, x, feet.y - h * 0.42f, bh, drop_ > 0.72f ? PAL_ALERT : PAL_CHAIN, false, fog, false);
    }
    if (locked_) {
        Spot bar = project(curve(stopZ_), stopZ_);
        if (bar.ok) {
            float bw = (kRoadHalf * 1.7f) * bar.ppm;
            float bh = std::max(3.f, bar.ppm * 0.35f);
            sprBox(art_.chain, bar.x + shx, bar.y - bh, bw, bh, PAL_CHAIN, fogFor(stopZ_));
        }
    }
    Spot line = project(curve(kLine), kLine);
    if (!line.ok) return;
    float lineW = (kRoadHalf * 1.8f) * line.ppm;
    sprBox(art_.stripe, line.x + shx, line.y, lineW, std::max(2.f, line.ppm * 0.1f), PAL_ALERT, fogFor(kLine));
}

void Game::drawProp(const Prop& pr, float shx) {
    Spot s = project(curve(pr.z) + pr.lat, pr.z);
    if (!s.ok) return;
    int fog = fogFor(pr.z);
    float h = std::clamp(pr.h * s.ppm, 5.f, 170.f);
    float x = s.x + shx;
    const gs::Mipped* m = &art_.pier;
    int pal = PAL_STONE;
    if (pr.kind == 1) {
        m = &art_.lamp;
        pal = PAL_LAMP;
    } else if (pr.kind == 2) {
        m = &art_.rail;
        pal = PAL_STONE;
    }
    spr(*m, x, s.y, h, pal, pr.lat > 0, fog, true);
}

void Game::drawLorry(const Lorry& r, float shx) {
    if (r.z < kZNear + 0.2f) return;
    Spot s = project(curve(r.z) + r.lat, r.z);
    if (!s.ok) return;
    int fog = fogFor(r.z);
    float h = std::clamp(kLorryH * s.ppm, 4.f, 120.f);
    float x = s.x + shx;
    spr(art_.shadow, x, s.y, h * 0.2f, PAL_SMOKE, false, 0, false);
    spr(art_.lorry, x, s.y, h, PAL_LORRY, false, fog, true);
    if (r.slot == 0) {
        float bob = std::sin(t_ * 3.5f) * 1.1f;
        spr(art_.pennant, x - h * 0.15f, s.y - h + bob, h * 0.34f, PAL_ALERT, false, fog, true);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    float shx = 0.f;
    if (shake_ > 0.f) shx = std::sin(t_ * 58.f) * 3.2f * std::min(shake_, 1.f);
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
    layRoad(shx);

    if (mode_ == Mode::Title) text("VIADUCT COLUMN", 160.f + shx, 24.f, 0.95f, PAL_MIST);
    else if (mode_ == Mode::Victory) text("ON THE ROAD", 160.f + shx, 26.f, 1.05f, PAL_GOOD);
    else if (mode_ == Mode::Fail) text("WATCH OVER", 160.f + shx, 26.f, 1.05f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx, 26.f, 1.1f, PAL_MIST);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    for (int i = 0; i < int(lorries_.size()); ++i) items.push_back({lorries_[size_t(i)].z, 0, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 2, i});
    items.push_back({10.4f, 3, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });
    for (const Item& it : items) {
        if (it.kind == 3) drawWatch(shx);
        else if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot s = project(curve(f.z) + f.lat, f.z);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            float h = std::clamp(s.ppm * (0.8f + u * 1.3f), 4.f, 24.f);
            spr(art_.smoke, s.x + shx, s.y - u * 5.f, h, PAL_SMOKE, false, fogFor(f.z), false);
        } else if (it.kind == 2) drawProp(props_[size_t(it.id)], shx);
        else drawLorry(lorries_[size_t(it.id)], shx);
    }

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "STOP THE COLUMN ON THE ROAD", PAL_MIST);
        hudC(23, "MISS THAT AND THE WATCH IS OVER", PAL_ALERT);
        hudC(24, "LEFT RIGHT  MAN THE CROWN", PAL_TEXT);
        hudC(25, "DOWN DROPS THE CHAIN", PAL_GOOD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_MIST);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_TEXT);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_TEXT);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(24, "THE WATCH HOLDS", PAL_MIST);
        hudC(26, "START", PAL_TEXT);
    } else if (mode_ == Mode::Fail) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "THE WATCH IS OVER", PAL_TEXT);
        hudC(26, "START RETRIES", PAL_TEXT);
    } else {
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(1, 0, buf, PAL_TEXT);
        int left = std::max(0, int(std::ceil(kWatch - t_)));
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(30, 0, buf, left <= 6 ? PAL_ALERT : PAL_TEXT);
        const char* h = hint();
        int pal = PAL_TEXT;
        if (!std::strcmp(h, "DROP THE CHAIN") || !std::strcmp(h, "HOLD THE CHAIN")) pal = PAL_GOOD;
        else if (!std::strcmp(h, "MAN THE CROWN") || !std::strcmp(h, "WAIT FOR THE CROWN")) pal = PAL_MIST;
        else pal = PAL_ALERT;
        hudC(1, h, pal);
        const char* postName = post_ == 0 ? "NEAR PIER" : post_ == 2 ? "FAR PIER" : "THE CROWN";
        hud(1, 2, postName, post_ == kCrown ? PAL_GOOD : PAL_MIST);
        hud(31, 2, drop_ >= 0.72f ? "CHAIN" : "OPEN", drop_ >= 0.72f ? PAL_ALERT : PAL_TEXT);
        hudC(27, "LEFT RIGHT    DOWN CHAIN", PAL_TEXT);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        t_ += kDt;
        drop_ = 0.22f + 0.18f * std::sin(t_ * 1.3f);
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
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
    tickPuffs();
    serviceAudio();
    draw();
    if (won_) sys.setLight(40, 140, 80);
    else if (mode_ == Mode::Fail) sys.setLight(170, 28, 24);
    else if (mode_ == Mode::Play && drop_ >= 0.72f) sys.setLight(160, 36, 28);
    else sys.setLight(70, 110, 150);
}

}  // namespace vcol

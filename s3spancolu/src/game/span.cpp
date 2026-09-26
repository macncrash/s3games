#include "game/span.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <string>

namespace scol {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr int kColumn = 4;
constexpr float kHorizon = 76.f;
constexpr float kZScale = 250.f;
constexpr float kNear = 4.2f;
constexpr float kFar = 30.f;
constexpr float kOn = 26.5f;
constexpr float kLate = 12.f;
constexpr float kSpanLo = 6.5f;
constexpr float kSpanHi = 29.2f;
constexpr float kRoad = 0.93f;
constexpr float kBeam = 0.78f;
constexpr float kSpanRate = 0.62f;
constexpr float kBeamRate = 2.6f;
constexpr float kHalt0 = 8.0f;
constexpr float kSpace = 6.6f;
constexpr float kWait = 40.f;
constexpr float kTruckV = 3.6f;
constexpr float kWatch = 26.f;
constexpr float kBotHi = 23.5f;
constexpr float kBotLo = 12.5f;
constexpr float kBeamZ = 6.2f;

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

}  // namespace

int Game::column() const { return kColumn; }

int Game::marker() const {
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

float Game::bendAt(float row) const { return std::sin(row * 0.013f + 0.4f) * 7.f; }

float Game::halfAt(float row) const { return 20.f + row * 0.64f; }

int Game::horizon() const { return std::clamp(int(std::lround(kHorizon + shy_)), 52, 100); }

float Game::leaf(float z) const {
    if (z <= kNear || z >= kFar) return 0.f;
    float u = (z - kNear) / (kFar - kNear);
    float a = std::clamp((u - 0.06f) / 0.08f, 0.f, 1.f);
    float b = std::clamp((0.94f - u) / 0.08f, 0.f, 1.f);
    return a * b;
}

float Game::swingPx(float z) const { return leaf(z) * (1.f - span_) * 156.f; }

bool Game::onDeck() const { return span_ >= kRoad; }

bool Game::beamDown() const { return beam_ >= kBeam && onDeck(); }

float Game::leadZ() const {
    for (const Rig& r : rigs_)
        if (r.slot == 0) return r.z;
    return 999.f;
}

Game::Spot Game::spot(float u, float z, float base) const {
    Spot s;
    if (!(z > 0.9f)) return s;
    float row = kZScale / z;
    float nearRow = kZScale / kNear;
    float t = std::clamp(row / nearRow, 0.02f, 1.45f);
    s.h = std::max(4.f, base * std::pow(t, 0.72f));
    float along = std::clamp((z - kNear) / 36.f, 0.f, 1.f);
    s.x = 160.f + bendAt(row) + swingPx(z) + u * halfAt(row) + shx_;
    s.y = float(hor_) + row;
    s.fog = int(std::clamp(along * 9.f, 0.f, 8.f));
    s.ok = true;
    return s;
}

void Game::buildProps() {
    props_.clear();
    auto add = [&](float z, float u, float h, int kind) {
        Prop p;
        p.z = z;
        p.u = u;
        p.h = h;
        p.kind = kind;
        props_.push_back(p);
    };
    add(4.6f, -1.18f, 108.f, 0);
    add(4.6f, 1.18f, 108.f, 0);
    add(28.5f, -1.22f, 86.f, 1);
    add(28.5f, 1.22f, 86.f, 1);
    add(5.1f, -1.55f, 52.f, 2);
    add(16.f, -1.7f, 28.f, 3);
    add(12.f, 1.85f, 30.f, 3);
    add(20.f, 1.65f, 24.f, 3);
}

void Game::lay(bool scenic) {
    rigs_.clear();
    puffs_.clear();
    for (int i = 0; i < kColumn; ++i) {
        Rig r;
        r.slot = i;
        r.cruise = kTruckV;
        r.haltZ = kHalt0 + float(i) * kSpace;
        r.z = scenic ? (18.f + float(i) * kSpace) : (kWait + float(i) * kSpace);
        r.speed = 0.f;
        rigs_.push_back(r);
    }
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    rolling_ = false;
    committed_ = false;
    seated_ = false;
    chimed_ = false;
    stopped_ = 0;
    through_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    t_ = 0;
    span_ = 0.22f;
    beam_ = 0.f;
    fanStep_ = -1;
    reason_ = "THE SPAN IS OPEN";
    lay(true);
}

void Game::begin() {
    lay(false);
    mode_ = Mode::Play;
    won_ = false;
    over_ = false;
    rolling_ = false;
    committed_ = false;
    seated_ = false;
    chimed_ = false;
    stopped_ = 0;
    through_ = 0;
    settle_ = 0;
    hold_ = 0;
    shake_ = 0;
    t_ = 0;
    span_ = 0.f;
    beam_ = 0.f;
    fanStep_ = -1;
    reason_ = "THE SPAN IS OPEN";
    blip(480.f);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    buildProps();
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.apu.setMaster(0.85f);
    sys.apu.setEcho(0.22f, 0.20f, 0.12f);
    if (bot_) begin();
    else bootTitle();
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.06f);
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
    hold_ = 1.5f;
    shake_ = 1.f;
    fanfare(false);
    sys_->apu.noiseBurst(0.2f, 180.f, 0.24f);
    sys_->rumble(0.5f, 0.18f, 160);
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    stopped_ = kColumn;
    reason_ = "THE COLUMN STOPS ON THE ROAD";
    hold_ = 1.3f;
    fanfare(true);
    sys_->rumble(0.25f, 0.48f, 150);
}

void Game::decide() {
    if (!rolling_) {
        lose("NOT ON THE SPAN");
        return;
    }
    float z = leadZ();
    if (z > kOn) lose("NOT ON THE SPAN");
    else if (z < kLate) lose("THE COLUMN LEFT THE SPAN");
    else {
        committed_ = true;
        for (Rig& r : rigs_) r.orderly = true;
        blip(160.f);
        sys_->rumble(0.18f, 0.32f, 70);
    }
}

const char* Game::hint() const {
    if (committed_) return "HOLD THE BEAM";
    if (beam_ > 0.4f && (!rolling_ || leadZ() > kOn)) return "LIFT THE BEAM";
    if (!onDeck()) return "CLOSE THE SPAN";
    if (leadZ() > kOn) return "LET THEM ON";
    if (leadZ() < kLate) return "TOO LATE";
    return "DROP THE BEAM";
}

void Game::stepRig(Rig& r, bool hold) {
    if (r.orderly && hold) {
        float dist = r.z - r.haltZ;
        if (dist <= 0.18f || (r.stopped && dist < 0.4f)) {
            if (!r.stopped) blip(88.f + float(r.slot) * 20.f);
            r.z = r.haltZ;
            r.speed = 0.f;
            r.stopped = true;
            return;
        }
        if (dist > 9.f) r.speed = std::min(r.cruise, r.speed + 4.f * kDt);
        else {
            float a = (r.speed * r.speed) / (2.f * std::max(dist, 0.25f));
            a = std::min(a, 22.f);
            r.speed = std::max(0.f, r.speed - a * kDt);
            if (r.speed > 0.55f && r.puff <= 0.f && puffs_.size() < 14) {
                r.puff = 0.14f;
                Puff p;
                p.z = r.z + 0.6f;
                p.age = 0.f;
                p.life = 0.42f;
                puffs_.push_back(p);
            }
        }
        r.z -= r.speed * kDt;
        if (r.z <= r.haltZ) {
            if (!r.stopped) blip(88.f + float(r.slot) * 20.f);
            r.z = r.haltZ;
            r.speed = 0.f;
            r.stopped = true;
        }
        return;
    }
    if (!rolling_) {
        r.speed = std::max(0.f, r.speed - 7.f * kDt);
        r.z -= r.speed * kDt;
        r.stopped = false;
        return;
    }
    r.stopped = false;
    r.speed = std::min(r.cruise, r.speed + 3.2f * kDt);
    r.z -= r.speed * kDt;
    if (r.z < kNear) {
        ++through_;
        lose("THE COLUMN LEFT THE SPAN");
    }
}

void Game::update() {
    t_ += kDt;
    float dir = 0.f;
    bool want = false;
    if (bot_) {
        bool window = rolling_ && !committed_ && leadZ() <= kBotHi && leadZ() >= kBotLo;
        want = committed_ || window;
        if (span_ < 0.995f) dir = 1.f;
    } else {
        const gs::Pad& pad = sys_->pad;
        if (std::fabs(pad.axisX) > 0.2f) dir = std::clamp(pad.axisX, -1.f, 1.f);
        else dir = float(pad.down(gs::BTN_RIGHT)) - float(pad.down(gs::BTN_LEFT));
        want = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_C) || pad.down(gs::BTN_A) || pad.down(gs::BTN_TURBO) ||
               pad.axisY < -0.45f;
    }
    float prevSpan = span_;
    span_ = std::clamp(span_ + dir * kSpanRate * kDt, 0.f, 1.f);
    motor_ = std::fabs(span_ - prevSpan) > 0.0001f ? 1.f : 0.f;
    float beamDir = want ? 1.f : -1.f;
    beam_ = std::clamp(beam_ + beamDir * kBeamRate * kDt, 0.f, 1.f);

    bool seat = beamDown();
    bool wasRolling = rolling_;
    if (!rolling_ && onDeck() && !seat && !committed_) rolling_ = true;
    if (rolling_ && !wasRolling) blip(540.f);

    if (rolling_ && span_ < kRoad - 0.04f) {
        bool lip = committed_ || leadZ() <= kFar + 1.2f;
        if (lip) {
            lose("THE SPAN OPENED");
            return;
        }
        rolling_ = false;
    }
    if (mode_ != Mode::Play) return;

    if (seat && !seated_) decide();
    seated_ = seat;
    if (mode_ != Mode::Play) return;

    if (committed_ && !seat) {
        committed_ = false;
        for (Rig& r : rigs_) {
            r.orderly = false;
            r.stopped = false;
        }
    }

    if (!chimed_ && rolling_ && leadZ() <= kFar) {
        chimed_ = true;
        blip(740.f);
    }

    bool hold = committed_ && seat;
    for (Rig& r : rigs_) {
        if (mode_ != Mode::Play) break;
        stepRig(r, hold);
        if (r.puff > 0.f) r.puff -= kDt;
    }
    if (mode_ != Mode::Play) return;

    int held = 0;
    for (const Rig& r : rigs_) {
        if (!r.stopped) continue;
        if (r.z >= kSpanLo && r.z <= kSpanHi) ++held;
    }
    stopped_ = held;
    if (hold && onDeck() && through_ == 0 && held == kColumn) settle_ += kDt;
    else settle_ = 0.f;
    if (settle_ >= 0.32f) win();
    else if (t_ > kWatch) lose("THE COLUMN DID NOT STOP");
}

void Game::tickPuffs() {
    for (Puff& p : puffs_) p.age += kDt;
    puffs_.erase(std::remove_if(puffs_.begin(), puffs_.end(), [](const Puff& p) { return p.age >= p.life; }),
                 puffs_.end());
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= kDt;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    bool moving = false;
    if (mode_ == Mode::Play) {
        for (const Rig& r : rigs_)
            if (r.speed > 0.4f) moving = true;
    }
    if (mode_ == Mode::Play && motor_ > 0.f) sys_->apu.tone(2, 62.f + span_ * 46.f, 0.035f);
    else if (moving) sys_->apu.tone(2, 46.f, 0.028f);
    else sys_->apu.tone(2, 0.f, 0.f);

    if (fanStep_ < 0) return;
    fanT_ += kDt;
    if (fanT_ < 0.13f) return;
    static const float good[] = {330.f, 415.3f, 523.3f, 659.3f};
    static const float bad[] = {220.f, 174.6f, 146.8f};
    const float* notes = fanGood_ ? good : bad;
    int n = fanGood_ ? 4 : 3;
    if (fanStep_ < n) sys_->apu.tone(1, notes[fanStep_], 0.08f);
    else sys_->apu.tone(1, 0.f, 0.f);
    ++fanStep_;
    fanT_ = 0.f;
    if (fanStep_ > n + 3) fanStep_ = -1;
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

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 4, 8);
    uint16_t skyHor = mode_ == Mode::Fail ? gs::rgb4(10, 4, 3) : gs::rgb4(14, 10, 7);
    v.setFogColor(skyHor);
    v.roadTime = int(t_ * 48.f);
    hor_ = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor_) {
            float t = float(y) / float(std::max(hor_, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor_);
        float z = kZScale / std::max(row, 0.5f);
        bool span = z > kNear && z < kFar;
        r.on = true;
        r.cx = 160.f + bendAt(row) + swingPx(z) + shx_;
        r.hw = halfAt(row);
        r.v = z * 48.f;
        r.pal = uint8_t(PAL_FIELD);
        r.style = z >= kFar ? 0 : 1;
        r.band = (int(std::floor(z * 0.55f)) & 1) ? 1 : 0;
        r.left = r.right = span ? gs::GROUND_WATER : gs::GROUND_LAND;
        v.lineFog[y] = uint8_t(std::clamp(int(8.f - row * 0.07f), 0, 8));
        float dropT = std::clamp(row / 130.f, 0.f, 1.f);
        v.lineBackdrop[y] = mix(gs::rgb4(2, 5, 8), gs::rgb4(1, 2, 4), dropT);
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0.f;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 68.f) * 4.2f * shake_;
        shy_ = std::cos(t_ * 49.f) * 2.2f * shake_;
    }
    layRoad();

    const char* banner = nullptr;
    int bannerPal = PAL_AMBER;
    float bannerScale = 1.1f;
    if (mode_ == Mode::Title) banner = "ONE SPAN";
    else if (mode_ == Mode::Victory) {
        banner = "ON THE ROAD";
        bannerPal = PAL_GOOD;
        bannerScale = 1.0f;
    } else if (mode_ == Mode::Fail) {
        banner = reason_;
        bannerPal = PAL_ALERT;
        bannerScale = std::strlen(reason_) > 16 ? 0.62f : 0.85f;
    } else if (mode_ == Mode::Pause) banner = "PAUSED";
    if (banner) text(banner, 160.f + shx_, 28.f, bannerScale, bannerPal);

    struct Item {
        float z;
        int kind;
        int id;
    };
    std::vector<Item> items;
    items.reserve(32);
    for (int i = 0; i < int(rigs_.size()); ++i) items.push_back({rigs_[size_t(i)].z, 0, i});
    for (int i = 0; i < int(puffs_.size()); ++i) items.push_back({puffs_[size_t(i)].z, 1, i});
    for (int i = 0; i < int(props_.size()); ++i) items.push_back({props_[size_t(i)].z, 2, i});
    items.push_back({kBeamZ, 3, 0});
    items.push_back({5.4f, 4, 0});
    std::sort(items.begin(), items.end(), [](const Item& a, const Item& b) { return a.z < b.z; });

    for (const Item& it : items) {
        if (it.kind == 4) {
            Spot s = spot(-1.42f, 5.2f, 70.f);
            if (!s.ok) continue;
            int fr = (motor_ > 0.f && (int(t_ * 8.f) & 1)) ? 1 : 0;
            spr(art_.hand[fr], s.x, s.y, s.h, PAL_HAND, false, 0, true);
            continue;
        }
        if (it.kind == 3) {
            Spot post = spot(-1.02f, kBeamZ, 64.f);
            Spot mid = spot(0.f, kBeamZ, 20.f);
            if (!post.ok || !mid.ok) continue;
            float row = kZScale / kBeamZ;
            float roadW = halfAt(row) * 1.85f;
            float w = 14.f + (roadW - 14.f) * beam_;
            float h = 58.f + (12.f - 58.f) * beam_;
            float cx = post.x + (mid.x - post.x) * beam_;
            float cy = (post.y - post.h * 0.72f) + ((mid.y - 8.f) - (post.y - post.h * 0.72f)) * beam_;
            sprBox(art_.beam, cx, cy, w, std::max(8.f, h), PAL_BEAM, 0);
            continue;
        }
        if (it.kind == 1) {
            const Puff& f = puffs_[size_t(it.id)];
            Spot s = spot(0.f, f.z, 26.f);
            if (!s.ok) continue;
            float u = f.age / std::max(0.05f, f.life);
            spr(art_.dust, s.x, s.y - u * 10.f, s.h * (0.6f + u), PAL_FX, false, s.fog, false);
            continue;
        }
        if (it.kind == 2) {
            const Prop& pr = props_[size_t(it.id)];
            float u = pr.u;
            float z = pr.z;
            // Buoys sit in the water, not on the swinging leaf.
            Spot s = spot(u, z, pr.h);
            if (pr.kind == 3) {
                float row = kZScale / z;
                s.x = 160.f + bendAt(row) + u * halfAt(row) + shx_;
            }
            if (!s.ok) continue;
            if (pr.kind == 2) {
                spr(art_.cabin, s.x, s.y, s.h, PAL_CABIN, false, s.fog, true);
            } else if (pr.kind == 3) {
                float bob = std::sin(t_ * 2.2f + z) * 2.f;
                spr(art_.buoy, s.x, s.y + bob, s.h, PAL_BUOY, u < 0, s.fog, true);
            } else if (pr.kind == 1) {
                spr(art_.tower, s.x, s.y, s.h, PAL_STONE, u > 0, s.fog, true);
            } else {
                spr(art_.pier, s.x, s.y, s.h, PAL_STONE, u > 0, s.fog, true);
                int lampPal = PAL_AMBER;
                if (mode_ == Mode::Victory) lampPal = PAL_GOOD;
                else if (mode_ == Mode::Fail) lampPal = PAL_ALERT;
                else if (beamDown()) lampPal = PAL_ALERT;
                else if (onDeck()) lampPal = (int(t_ * 3.f) & 1) ? PAL_AMBER : PAL_GOOD;
                spr(art_.lamp, s.x, s.y - s.h * 0.96f, std::max(7.f, s.h * 0.12f), lampPal, false, 0, false);
            }
            continue;
        }
        const Rig& r = rigs_[size_t(it.id)];
        Spot s = spot(0.f, r.z, 76.f);
        if (!s.ok) continue;
        float bob = (r.speed > 0.3f) ? std::sin(t_ * 9.f + r.z) * 1.2f : 0.f;
        spr(art_.shadow, s.x, s.y, s.h * 0.28f, PAL_FX, false, s.fog, false);
        spr(art_.truck, s.x, s.y + bob, s.h, PAL_TRUCK, false, s.fog, true);
    }

    spr(art_.trees, 70.f + shx_ * 0.2f, float(hor_) + 2.f, 34.f, PAL_CABIN, false, 4, true);
    spr(art_.trees, 250.f + shx_ * 0.2f, float(hor_) + 4.f, 30.f, PAL_CABIN, true, 5, true);
    float drift = std::fmod(t_ * 5.f, 400.f);
    spr(art_.cloud, drift - 50.f, 18.f, 14.f, PAL_FX, false, 2, false);
    spr(art_.cloud, std::fmod(drift + 210.f, 400.f) - 40.f, 30.f, 11.f, PAL_FX, true, 3, false);
    spr(art_.sun, 248.f, 20.f, 20.f, PAL_FX, false, 0, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(22, "STOP THE COLUMN ON THE ROAD", PAL_AMBER);
        hudC(23, "CLOSE THE SPAN, THEN DROP THE BEAM", PAL_GOOD);
        hudC(24, "RIGHT CLOSES    DOWN OR C DROPS", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(26, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 27, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Victory) {
        hudC(23, "THE COLUMN STOPS ON THE ROAD", PAL_GOOD);
        hudC(24, "THEN IT IS DONE", PAL_AMBER);
        hudC(26, "START", PAL_HUD);
    } else if (mode_ == Mode::Fail) {
        hudC(23, reason_, PAL_ALERT);
        hudC(24, "THE COLUMN IS NOT ON THE ROAD", PAL_HUD);
        hudC(26, "START RETRIES", PAL_HUD);
    } else {
        hud(1, 0, onDeck() ? "SPAN SET" : "SPAN OPEN", onDeck() ? PAL_GOOD : PAL_AMBER);
        std::snprintf(buf, sizeof buf, "COLUMN %d/%d", stopped_, kColumn);
        hud(40 - int(std::strlen(buf)) - 1, 0, buf, PAL_HUD);
        hud(1, 1, beamDown() ? "BEAM ACROSS" : "BEAM UP", beamDown() ? PAL_ALERT : PAL_HUD);
        const char* h = hint();
        int pal = PAL_HUD;
        if (!std::strcmp(h, "DROP THE BEAM") || !std::strcmp(h, "CLOSE THE SPAN")) pal = PAL_AMBER;
        else if (!std::strcmp(h, "HOLD THE BEAM") || !std::strcmp(h, "TOO LATE") || !std::strcmp(h, "LIFT THE BEAM"))
            pal = PAL_ALERT;
        else if (!std::strcmp(h, "LET THEM ON")) pal = PAL_GOOD;
        hudC(2, h, pal);
        hudC(27, "RIGHT CLOSES    DOWN OR C HOLDS", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - kDt * 0.85f);
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        t_ += kDt;
        span_ = 0.18f + 0.62f * (0.5f + 0.5f * std::sin(t_ * 0.7f));
        beam_ = 0.08f + 0.12f * (0.5f + 0.5f * std::sin(t_ * 1.3f));
        motor_ = 0.4f;
        if (pad.pressed(gs::BTN_START)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            motor_ = 0.f;
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        motor_ = 0.f;
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
    else if (mode_ == Mode::Fail) sys.setLight(170, 36, 28);
    else if (mode_ == Mode::Play && beamDown()) sys.setLight(170, 48, 30);
    else if (mode_ == Mode::Play && onDeck()) sys.setLight(40, 120, 150);
    else sys.setLight(40, 80, 130);
}

}  // namespace scol

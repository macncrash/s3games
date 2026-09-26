#include "game/door.h"

#include "version.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace rdoor {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kMove = 5.6f;
constexpr float kAlign = 0.34f;
constexpr float kZ0 = 13.5f;
constexpr int kWatchFrames = 180 * 60;
constexpr int kHor = 56;
constexpr float kDoorY = 158.f;
constexpr float kDoorH = 118.f;
constexpr float kKeepH = 86.f;

uint16_t mix(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

float speedFor(int frame) {
    if (frame < 50 * 60) return 2.7f;
    if (frame < 110 * 60) return 3.2f;
    return 3.9f;
}

gs::FMPatch creakPatch() {
    gs::FMPatch p;
    p.alg = 2;
    p.fb = 0.4f;
    p.op[0] = {1.0f, 0.75f, 0.05f, 0.4f, 0.8f, 0.45f};
    p.op[1] = {2.0f, 0.3f, 0.08f, 0.45f, 0.55f, 0.4f};
    p.op[2] = {0.5f, 0.4f, 0.1f, 0.5f, 0.65f, 0.4f};
    p.op[3] = {1.0f, 0.22f, 0.06f, 0.3f, 0.45f, 0.3f};
    p.vol = 0.06f;
    p.drive = 0.2f;
    p.tone = 520;
    return p;
}

const char* postName(int lane) {
    if (lane < 0) return "LEFT POST";
    if (lane > 0) return "RIGHT POST";
    return "THE BAR";
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won || mode_ == Mode::Lost) return 4;
    if (mode_ != Mode::Play && mode_ != Mode::Pause) return 0;
    if (watch_ > 120.f) return 3;
    if (blockFlash_ > 0.15f) return 2;
    return 1;
}

int Game::horizon() const { return std::clamp(int(std::lround(float(kHor) + shy_)), 40, 90); }

float Game::bend(float row) const { return std::sin(row * 0.018f + scroll_ * 0.01f) * (5.f + row * 0.025f); }

bool Game::alignedTo(int lane) const { return std::fabs(u_ - float(lane)) <= kAlign; }

int Game::soonest(int skipLane) const {
    int best = -1;
    float eta = 1e9f;
    for (int i = 0; i < 6; ++i) {
        if (!foes_[i].on || foes_[i].lane == skipLane) continue;
        float e = foes_[i].z / std::max(foes_[i].speed, 0.1f);
        if (e < eta) {
            eta = e;
            best = i;
        }
    }
    return best;
}

void Game::blip(int ch, float freq, float hold) {
    sys_->apu.tone(ch, freq, 0.055f);
    if (ch == 0) beep0_ = hold;
    else beep2_ = hold;
}

void Game::puffAt(float x, float y, int n) {
    for (int k = 0; k < n; ++k) {
        for (Puff& p : puffs_) {
            if (p.t > 0.f) continue;
            float ang = float((age_ + k * 3) % 8) * 0.8f;
            p.x = x;
            p.y = y;
            p.vx = std::sin(ang) * (18.f + float(k) * 6.f);
            p.vy = -16.f - float(k) * 5.f;
            p.t = 0.38f;
            break;
        }
    }
}

void Game::schedule() {
    waveCount_ = 0;
    gustCount_ = 0;
    auto wave = [&](float sec, int lane) {
        if (waveCount_ >= 64) return;
        int f = int(std::lround(sec * 60.f));
        if (f < 40 || f > kWatchFrames - 80) return;
        waves_[waveCount_++] = Wave{f, lane};
    };
    auto gust = [&](float sec, float dur) {
        if (gustCount_ >= 8) return;
        gusts_[gustCount_++] = Gust{int(std::lround(sec * 60.f)), std::max(1, int(std::lround(dur * 60.f)))};
    };
    auto crowded = [](float sec) {
        return (sec > 72.f && sec < 84.f) || (sec > 122.f && sec < 134.f) || (sec > 146.f && sec < 158.f);
    };

    const int laneSeq[] = {0, -1, 1, 0, 1, -1};
    float t = 8.f;
    int n = 0;
    while (t < 165.f && n < 40) {
        if (crowded(t)) {
            t += 6.4f;
            ++n;
            continue;
        }
        int lane = laneSeq[n % 6];
        wave(t, lane);
        if (t > 46.f && (n % 4 == 3)) {
            int other = laneSeq[(n + 2) % 6];
            if (other == lane) other = lane == 0 ? -1 : 0;
            wave(t + 1.4f, other);
            t += 8.4f;
        } else {
            t += 6.6f;
        }
        ++n;
    }
    // Opposite posts, half a second apart: the wedge has to take one of them.
    wave(76.0f, -1);
    wave(76.5f, 1);
    wave(126.0f, 1);
    wave(126.5f, -1);
    wave(150.0f, -1);
    wave(150.5f, 1);
    wave(169.0f, -1);
    wave(171.3f, 1);
    wave(173.6f, 0);

    gust(22.f, 1.7f);
    gust(43.f, 1.7f);
    gust(64.f, 1.8f);
    gust(90.f, 1.7f);
    gust(112.f, 1.8f);
    gust(138.f, 1.7f);
    gust(159.f, 1.6f);

    std::sort(waves_, waves_ + waveCount_, [](const Wave& a, const Wave& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        return a.lane < b.lane;
    });
}

void Game::bootTitle() {
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    holding_ = false;
    stamLock_ = false;
    wedging_ = false;
    reason_ = "THE WATCH IS OVER";
    age_ = 0;
    post_ = 0;
    heldDir_ = 0;
    wedgeOn_ = -1;
    blocks_ = 0;
    misses_ = 0;
    fanStep_ = -1;
    waveNext_ = 0;
    gustNext_ = 0;
    gustLeft_ = 0;
    watch_ = 0;
    give_ = 0.02f;
    u_ = 0;
    stam_ = 1;
    shake_ = 0;
    dirCool_ = 0;
    wedgeLife_ = 0;
    wedgeCd_ = 0;
    wedgeArm_ = 0;
    blockFlash_ = 0;
    for (Foe& f : foes_) f.on = false;
    for (Puff& p : puffs_) p.t = 0;
    if (sys_) {
        sys_->apu.silence();
        sys_->setLight(70, 50, 30);
    }
}

void Game::begin() {
    bootTitle();
    schedule();
    waveNext_ = 0;
    gustNext_ = 0;
    give_ = 0.06f;
    mode_ = Mode::Play;
    sys_->apu.setPatch(0, creakPatch());
    sys_->apu.keyOn(0, 84.f, 0.05f);
    sys_->setLight(120, 70, 30);
}

void Game::plant(int lane) {
    wedgeOn_ = lane;
    wedgeLane_ = lane;
    wedgeLife_ = 6.4f;
    wedgeCd_ = 7.2f;
    wedgeArm_ = 0;
    wedging_ = false;
    stam_ = std::max(0.f, stam_ - 0.05f);
    blip(0, 250.f, 0.06f);
    puffAt(160.f + float(lane) * 74.f, 188.f, 2);
}

void Game::spawn(int lane, float speed) {
    for (Foe& f : foes_) {
        if (f.on) continue;
        f.on = true;
        f.lane = lane;
        f.z = kZ0;
        f.speed = speed;
        return;
    }
}

void Game::strike(Foe& f) {
    bool covered = (wedgeOn_ == f.lane && wedgeLife_ > 0.f) || (holding_ && alignedTo(f.lane));
    if (wedgeOn_ == f.lane) {
        wedgeOn_ = -1;
        wedgeLife_ = 0;
        covered = true;
    }
    f.on = false;
    float x = 160.f + float(f.lane) * 74.f;
    if (covered) {
        ++blocks_;
        blockFlash_ = 0.45f;
        give_ = std::max(0.f, give_ - 0.02f);
        shake_ = std::max(shake_, 0.35f);
        blip(0, 340.f, 0.05f);
        sys_->apu.noiseBurst(0.18f, 420.f, 0.07f);
        puffAt(x, 150.f, 3);
    } else {
        ++misses_;
        float hit = 0.22f + 0.06f * std::clamp(watch_ / 180.f, 0.f, 1.f);
        give_ += hit;
        shake_ = 1.f;
        sys_->apu.noiseBurst(0.55f, 160.f, 0.2f);
        sys_->rumble(0.7f, 0.3f, 90);
        puffAt(x, 146.f, 5);
    }
}

void Game::win() {
    mode_ = Mode::Won;
    won_ = true;
    over_ = true;
    reason_ = "THE DOOR HELD";
    fanStep_ = 0;
    fanT_ = 0;
    holding_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.noise(0, 0);
    sys_->rumble(0.2f, 0.5f, 200);
    sys_->setLight(40, 120, 50);
}

void Game::lose() {
    mode_ = Mode::Lost;
    won_ = false;
    over_ = true;
    give_ = 1.f;
    reason_ = "THE DOOR SWUNG OPEN";
    fanStep_ = -1;
    holding_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.noise(0, 0);
    sys_->apu.noiseBurst(0.7f, 140.f, 0.4f);
    sys_->rumble(0.85f, 0.2f, 260);
    sys_->setLight(140, 20, 16);
}

void Game::botPlan(int& post, bool& hold, bool& wedge) {
    post = 0;
    hold = false;
    wedge = false;

    struct T {
        float eta;
        int lane;
        int id;
    };
    T ts[6];
    int n = 0;
    for (int i = 0; i < 6; ++i) {
        if (!foes_[i].on) continue;
        if (wedgeOn_ == foes_[i].lane && wedgeLife_ > 0.f) continue;
        ts[n++] = T{foes_[i].z / std::max(foes_[i].speed, 0.1f), foes_[i].lane, i};
    }
    std::sort(ts, ts + n, [](const T& a, const T& b) { return a.eta < b.eta; });

    auto travel = [&](int lane) { return std::fabs(u_ - float(lane)) / kMove; };
    auto between = [&](int a, int b) { return std::fabs(float(a - b)) / kMove; };

    if (wedgeOn_ >= 0) wedging_ = false;
    if (n >= 2 && wedgeOn_ < 0 && wedgeCd_ <= 0.f && stam_ > 0.22f) {
        float gap = ts[1].eta - ts[0].eta;
        float need = between(ts[0].lane, ts[1].lane) + 0.30f;
        float out = travel(ts[1].lane);
        float back = between(ts[1].lane, ts[0].lane);
        bool wouldMiss = gap < need && ts[0].lane != ts[1].lane;
        bool can = ts[0].eta > out + back + 0.42f && ts[0].eta > 0.95f && ts[1].eta < 8.f;
        if (!wedging_ && wouldMiss && can) {
            wedging_ = true;
            wedgeLane_ = ts[1].lane;
        }
    }
    if (wedging_) {
        if (n == 0 || ts[0].eta < 0.85f) wedging_ = false;
        else if (wedgeCd_ > 0.f && wedgeOn_ < 0) wedging_ = false;
    }

    if (wedging_) {
        post = wedgeLane_;
        if (alignedTo(post)) wedge = true;
    } else if (n > 0) {
        post = ts[0].lane;
    }

    bool gustSoon = gustLeft_ > 0;
    if (!gustSoon && gustNext_ < gustCount_) {
        int in = gusts_[gustNext_].frame - age_;
        if (in >= 0 && in < 18) gustSoon = true;
    }
    float nextEta = n > 0 ? ts[0].eta : 99.f;
    bool aligned = alignedTo(post);

    if (gustSoon && stam_ > 0.22f && nextEta > 0.7f) hold = true;
    if (n > 0 && aligned && nextEta < 1.35f && !wedging_) hold = true;
    if (n > 0 && alignedTo(ts[0].lane) && ts[0].eta < 1.35f) hold = true;
    if (n == 0 && give_ > 0.1f && stam_ > 0.55f && !gustSoon) hold = true;
    if (n > 0 && nextEta > 3.2f && give_ > 0.14f && stam_ > 0.62f && !wedging_) hold = true;
    if (stamLock_) hold = false;
    if (stam_ < 0.45f && nextEta > 2.4f && !gustSoon) hold = false;
}

void Game::update() {
    ++age_;
    watch_ = age_ * kDt;

    if (gustLeft_ > 0) --gustLeft_;
    if (gustLeft_ == 0 && gustNext_ < gustCount_ && age_ >= gusts_[gustNext_].frame) {
        gustLeft_ = gusts_[gustNext_].dur;
        ++gustNext_;
        blip(2, 180.f, 0.08f);
    }
    while (waveNext_ < waveCount_ && age_ >= waves_[waveNext_].frame) {
        spawn(waves_[waveNext_].lane, speedFor(waves_[waveNext_].frame));
        ++waveNext_;
    }

    int post = post_;
    bool wantHold = false;
    bool wantWedge = false;
    if (bot_) {
        botPlan(post, wantHold, wantWedge);
        post_ = post;
    } else {
        const gs::Pad& p = sys_->pad;
        int dir = 0;
        if (p.down(gs::BTN_LEFT)) dir -= 1;
        if (p.down(gs::BTN_RIGHT)) dir += 1;
        if (dir == 0) {
            if (p.axisX < -0.35f) dir = -1;
            else if (p.axisX > 0.35f) dir = 1;
        }
        if (p.pressed(gs::BTN_UP) || p.pressed(gs::BTN_DOWN)) post_ = 0;
        if (dir == 0) {
            heldDir_ = 0;
            dirCool_ = 0;
        } else if (dir != heldDir_) {
            post_ = std::clamp(post_ + dir, -1, 1);
            heldDir_ = dir;
            dirCool_ = 0.2f;
        } else {
            dirCool_ -= kDt;
            if (dirCool_ <= 0.f) {
                post_ = std::clamp(post_ + dir, -1, 1);
                dirCool_ = 0.16f;
            }
        }
        wantHold = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.accel > 0.4f;
        if (p.pressed(gs::BTN_A) || p.pressed(gs::BTN_B) || p.pressed(gs::BTN_X) || p.pressed(gs::BTN_Z))
            wedgeArm_ = 0.32f;
        wantWedge = wedgeArm_ > 0.f;
    }

    float goal = float(post_);
    float step = kMove * kDt;
    if (std::fabs(goal - u_) <= step) u_ = goal;
    else u_ += std::copysign(step, goal - u_);

    if (stamLock_) wantHold = false;
    if (wantHold && stam_ > 0.f) {
        holding_ = true;
        stam_ = std::max(0.f, stam_ - 0.12f * kDt);
        if (stam_ <= 0.f) stamLock_ = true;
    } else {
        holding_ = false;
        stam_ = std::min(1.f, stam_ + 0.24f * kDt);
        if (stamLock_ && stam_ >= 0.32f) stamLock_ = false;
    }

    if (wedgeArm_ > 0.f) wedgeArm_ -= kDt;
    if (wedgeCd_ > 0.f) wedgeCd_ -= kDt;
    if (wedgeLife_ > 0.f) {
        wedgeLife_ -= kDt;
        if (wedgeLife_ <= 0.f) {
            wedgeLife_ = 0;
            wedgeOn_ = -1;
        }
    }
    if (wantWedge && wedgeOn_ < 0 && wedgeCd_ <= 0.f && alignedTo(post_) && stam_ > 0.12f) plant(post_);

    for (Foe& f : foes_) {
        if (!f.on) continue;
        f.z -= f.speed * kDt;
        if (f.z <= 0.08f) strike(f);
    }

    float wind = watch_ > 120.f ? 0.0045f : 0.0032f;
    if (gustLeft_ > 0) wind += 0.11f;
    if (holding_) {
        wind = 0;
        give_ -= 0.055f * kDt;
    }
    give_ += wind * kDt;
    give_ = std::clamp(give_, 0.f, 1.f);

    if (blockFlash_ > 0.f) blockFlash_ -= kDt;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.04f);

    for (Puff& p : puffs_) {
        if (p.t <= 0.f) continue;
        p.t -= kDt;
        p.x += p.vx * kDt;
        p.y += p.vy * kDt;
        p.vy += 36.f * kDt;
    }

    float hz = 78.f + give_ * 150.f + (holding_ ? 22.f : 0.f);
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, 0.04f + give_ * 0.08f + (gustLeft_ > 0 ? 0.02f : 0.f));
    sys_->apu.noise(gustLeft_ > 0 ? 0.045f : 0.012f, gustLeft_ > 0 ? 900.f : 500.f);

    if (age_ % 60 == 0 && age_ < kWatchFrames) {
        blip(2, age_ > kWatchFrames - 600 ? 660.f : 392.f, 0.04f);
    }

    if (give_ >= 1.f) lose();
    else if (age_ >= kWatchFrames) win();
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(8, 6, 4));
    sys.apu.setMaster(0.8f);
    sys.apu.setEcho(0.14f, 0.2f, 0.1f);
    bootTitle();
    if (bot_) begin();
}

void Game::serviceAudio() {
    if (beep0_ > 0.f) {
        beep0_ -= kDt;
        if (beep0_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (beep2_ > 0.f) {
        beep2_ -= kDt;
        if (beep2_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
    if (fanStep_ >= 0 && (mode_ == Mode::Won)) {
        fanT_ += kDt;
        if (fanT_ >= 0.16f && fanStep_ < 4) {
            fanT_ = 0;
            static const float notes[] = {262.f, 330.f, 392.f, 523.f};
            sys_->apu.tone(1, notes[fanStep_], 0.07f);
            ++fanStep_;
            beep2_ = 0.14f;
        } else if (fanStep_ >= 4 && beep2_ <= 0.f) {
            sys_->apu.tone(1, 0, 0);
        }
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    scroll_ += 10.f * kDt;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        u_ = std::sin(t_ * 0.7f) * 0.12f;
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_TURBO)) begin();
        else if (pad.pressed(gs::BTN_MODE) && !bot_) sys.quit();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            holding_ = false;
            sys.apu.setVol(0, 0.02f);
        } else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            bootTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) mode_ = Mode::Play;
        else if (pad.pressed(gs::BTN_MODE)) bootTitle();
    } else {
        if (mode_ == Mode::Won) give_ = std::max(0.f, give_ - kDt * 0.4f);
        if (!bot_ && pad.pressed(gs::BTN_START)) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) bootTitle();
    }

    serviceAudio();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; ++i) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (x < 0 || x > 39 || c < 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    int n = int(std::strlen(s));
    hud(std::max(0, (40 - n) / 2), row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    float width = 0;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) width += 12.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c < 33 || c > 126) {
            x += 12.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, 0, false);
        x += gw;
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, int fog, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    stamp(m, cx, cy, h * float(m.w) / float(m.h), h, pal, flip, fog, shadow);
}

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip, int fog, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::layRoad() {
    gs::VDP& v = sys_->vdp;
    uint16_t skyTop = gs::rgb4(2, 3, 7);
    uint16_t skyHor = gs::rgb4(13, 8, 5);
    if (mode_ == Mode::Lost) skyHor = gs::rgb4(10, 3, 3);
    if (mode_ == Mode::Won) skyHor = gs::rgb4(14, 10, 5);
    v.setFogColor(skyHor);
    int hor = horizon();
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        gs::RoadLine& r = v.road[y];
        if (y <= hor) {
            float t = float(y) / float(std::max(hor, 1));
            v.lineBackdrop[y] = mix(skyTop, skyHor, t * t);
            v.lineFog[y] = 0;
            r.on = false;
            continue;
        }
        float row = float(y - hor);
        r.on = true;
        r.cx = 160.f + bend(row) + shx_;
        r.hw = 18.f + row * 0.92f;
        r.v = scroll_ + 5200.f / std::max(row, 1.f);
        r.pal = uint8_t(PAL_FIELD);
        r.style = gs::ROAD_ROCKY;
        r.band = (int(std::floor(row * 0.12f)) & 1) ? 1 : 0;
        r.left = r.right = gs::GROUND_DROP;
        v.lineFog[y] = uint8_t(std::clamp(int(10.f - row * 0.07f), 0, 10));
        v.lineBackdrop[y] = mix(gs::rgb4(3, 3, 4), gs::rgb4(1, 1, 2), std::clamp(row / 150.f, 0.f, 1.f));
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.A.enabled = false;
    v.B.enabled = false;
    shx_ = shy_ = 0;
    if (shake_ > 0.f) {
        shx_ = std::sin(t_ * 70.f) * 5.f * shake_;
        shy_ = std::cos(t_ * 54.f) * 2.6f * shake_;
    }
    layRoad();

    int hor = horizon();
    int flutter = int(t_ * (gustLeft_ > 0 ? 14.f : 6.f)) & 1;
    float open = give_ * 72.f;
    float leafH = kDoorH;
    float leafW = leafH * float(art_.leaf.w) / float(std::max(1, int(art_.leaf.h)));
    float leftX = 146.f - open + shx_;
    float rightX = 174.f + open + shx_;
    float postX = 160.f + u_ * 74.f + shx_;

    auto farSpot = [&](float lane, float z, float base, float& x, float& y, float& h, int& fog, bool& ok) {
        float nz = std::clamp(z / kZ0, 0.f, 1.f);
        float row = 10.f + (1.f - nz) * (1.f - nz) * 62.f;
        y = float(hor) + row + shy_;
        h = std::max(8.f, base * (0.28f + 0.72f * (1.f - nz)));
        x = 160.f + bend(row) + lane * (16.f + row * 0.62f) + shx_;
        fog = int(std::clamp(nz * 8.f, 0.f, 8.f));
        ok = y < kDoorY - 8.f;
    };

    // Earlier sprites draw on top. Titles and the keeper sit in front of the gate;
    // the ridge sits behind the leaves.
    if (mode_ == Mode::Title) text("S3 RIDGE DOOR", 160.f + shx_, 30.f, 0.72f, PAL_AMBER);
    else if (mode_ == Mode::Won) text("THE DOOR HELD", 160.f + shx_, 28.f, 0.78f, PAL_GOOD);
    else if (mode_ == Mode::Lost) text("THE WATCH IS OVER", 160.f + shx_, 28.f, 0.62f, PAL_ALERT);
    else if (mode_ == Mode::Pause) text("PAUSED", 160.f + shx_, 32.f, 0.9f, PAL_AMBER);

    for (const Puff& p : puffs_) {
        if (p.t <= 0.f) continue;
        float k = std::clamp(p.t / 0.38f, 0.f, 1.f);
        spr(art_.dust, p.x + shx_, p.y, 12.f + (1.f - k) * 10.f, PAL_FX, false, int((1.f - k) * 4.f), false);
    }

    bool bracing = holding_ || mode_ == Mode::Won;
    spr(art_.shadow, postX, 204.f + shy_, 14.f, PAL_FX, false, 0, true);
    spr(art_.keeper[bracing ? 1 : int(t_ * 5.f) & 1], postX, 178.f + shy_ + (bracing ? 2.f : 0.f), kKeepH, PAL_YOU,
        u_ < -0.2f, 0, true);

    struct Near {
        int id;
        float z;
    };
    Near near[6];
    int nearN = 0;
    struct Far {
        float x, y, h;
        int fog;
        int id;
        bool foe;
        bool flip;
    };
    Far far[12];
    int farN = 0;
    for (int i = 0; i < 6; ++i) {
        if (!foes_[i].on) continue;
        if (foes_[i].z < 1.7f) near[nearN++] = Near{i, foes_[i].z};
        else {
            float x, y, h;
            int fog;
            bool ok = false;
            farSpot(float(foes_[i].lane) * 0.82f, foes_[i].z, 70.f, x, y, h, fog, ok);
            if (!ok || farN >= 12) continue;
            far[farN++] = Far{x, y, h, fog, i, true, foes_[i].lane > 0};
        }
    }
    std::sort(near, near + nearN, [](const Near& a, const Near& b) { return a.z < b.z; });
    for (int i = 0; i < nearN; ++i) {
        const Foe& f = foes_[near[i].id];
        float k = std::clamp(1.f - f.z / 1.7f, 0.f, 1.f);
        float x = 160.f + float(f.lane) * 74.f + shx_;
        float h = 62.f + k * 22.f;
        float y = kDoorY + 8.f - k * 6.f + shy_;
        spr(art_.shadow, x, y + h * 0.42f, 12.f, PAL_FX, false, 0, true);
        spr(art_.raider[int(t_ * 8.f + f.z) & 1], x, y, h, PAL_FOE, f.lane > 0, 0, true);
    }

    if (wedgeOn_ >= 0) {
        float wx = 160.f + float(wedgeOn_) * 74.f + shx_;
        spr(art_.wedge, wx, 196.f + shy_, 16.f, PAL_DOOR, wedgeOn_ > 0, 0, false);
    }
    stamp(art_.sill, 160.f + shx_, 206.f + shy_, 250.f, 20.f, PAL_STONE, false, 0, false);
    float barY = kDoorY - 8.f - give_ * 18.f + shy_;
    stamp(art_.bar, 160.f + shx_, barY, 132.f * (1.f - give_ * 0.35f), 12.f, PAL_DOOR, false, 0, false);
    spr(art_.pennant[flutter], 196.f + shx_ + (gustLeft_ > 0 ? 6.f : 0.f), kDoorY - leafH * 0.52f + shy_, 18.f,
        PAL_ALERT, false, 0, false);
    stamp(art_.jamb, 58.f + shx_, kDoorY + 6.f + shy_, 34.f, leafH + 8.f, PAL_STONE, false, 0, false);
    stamp(art_.jamb, 262.f + shx_, kDoorY + 6.f + shy_, 34.f, leafH + 8.f, PAL_STONE, true, 0, false);
    stamp(art_.lintel, 160.f + shx_, kDoorY - leafH * 0.46f + shy_, leafW * 2.15f + 28.f, 28.f, PAL_STONE, false, 0,
          false);
    spr(art_.leaf, leftX, kDoorY + shy_, leafH, PAL_DOOR, false, 0, true);
    spr(art_.leaf, rightX, kDoorY + shy_, leafH, PAL_DOOR, true, 0, true);

    if (mode_ == Mode::Title && farN < 12) {
        float x, y, h;
        int fog;
        bool ok = false;
        farSpot(-0.55f, 7.2f + std::sin(t_ * 0.8f) * 0.2f, 64.f, x, y, h, fog, ok);
        if (ok) far[farN++] = Far{x, y, h, fog, 0, true, false};
    }
    const Rock rocks[] = {{4.2f, -1.15f, 30.f}, {7.4f, 1.2f, 24.f}, {10.2f, -1.25f, 28.f}, {12.4f, 1.15f, 20.f}};
    for (const Rock& rk : rocks) {
        if (farN >= 12) break;
        float x, y, h;
        int fog;
        bool ok = false;
        farSpot(rk.lane, rk.z, rk.h, x, y, h, fog, ok);
        if (ok) far[farN++] = Far{x, y, h, fog, 0, false, rk.lane > 0};
    }
    std::sort(far, far + farN, [](const Far& a, const Far& b) { return a.y > b.y; });
    for (int i = 0; i < farN; ++i) {
        const Far& f = far[i];
        if (f.foe) {
            spr(art_.shadow, f.x, f.y + 2.f, f.h * 0.16f, PAL_FX, false, f.fog, true);
            int step = int(t_ * 6.f + float(f.id)) & 1;
            spr(art_.raider[step], f.x, f.y, f.h, PAL_FOE, f.flip, f.fog, true);
        } else {
            spr(art_.rock, f.x, f.y, f.h, PAL_STONE, f.flip, f.fog, true);
        }
    }

    spr(art_.peak[0], 62.f + shx_ * 0.2f, float(hor) + 2.f, 62.f, PAL_MOUNT, false, 2, true);
    spr(art_.peak[1], 250.f + shx_ * 0.2f, float(hor) + 6.f, 46.f, PAL_MOUNT, false, 4, true);
    float drift = std::fmod(t_ * 7.f, 380.f);
    spr(art_.cloud, drift - 40.f, 24.f, 16.f, PAL_FX, false, 4, false);
    spr(art_.cloud, std::fmod(drift + 190.f, 380.f) - 30.f, 38.f, 12.f, PAL_FX, true, 5, false);
    spr(art_.sun, 248.f + shx_ * 0.15f, 28.f, 22.f, PAL_FX, false, 0, false);

    char buf[48];
    if (mode_ == Mode::Title) {
        hudC(21, "HOLD THE DOOR FOR THREE MINUTES", PAL_AMBER);
        hudC(22, "MISS THAT AND THE WATCH IS OVER", PAL_HUD);
        hudC(24, "ARROWS STEP THE POSTS", PAL_HUD);
        hudC(25, "C OR SPACE HOLDS THE BAR", PAL_AMBER);
        hudC(26, "Z OR X PLANTS A WEDGE", PAL_HUD);
        if ((int(t_ * 2.f) & 1) == 0) hudC(27, "PRESS START", PAL_AMBER);
        hud(39 - int(std::strlen(S3_VERSION_STRING)), 0, S3_VERSION_STRING, PAL_HUD);
    } else if (mode_ == Mode::Pause) {
        hudC(24, "START RESUMES", PAL_HUD);
        hudC(26, "ESC TITLE", PAL_HUD);
    } else if (mode_ == Mode::Won) {
        hudC(23, "THE DOOR HELD FOR THREE MINUTES", PAL_GOOD);
        hudC(24, "THE WATCH HELD", PAL_AMBER);
        std::snprintf(buf, sizeof buf, "BLOCKS %d", blocks_);
        hudC(25, buf, PAL_HUD);
        hudC(27, "START", PAL_HUD);
    } else if (mode_ == Mode::Lost) {
        hudC(23, "THE WATCH IS OVER", PAL_ALERT);
        hudC(24, reason_, PAL_HUD);
        hudC(26, "START RETRIES", PAL_HUD);
    } else {
        int left = std::max(0, (kWatchFrames - age_ + 59) / 60);
        std::snprintf(buf, sizeof buf, "WATCH %d:%02d", left / 60, left % 60);
        hud(1, 0, buf, left <= 15 ? PAL_ALERT : PAL_HUD);
        int seal = int(std::lround((1.f - give_) * 100.f));
        seal = std::clamp(seal, 0, 100);
        std::snprintf(buf, sizeof buf, "DOOR %d", seal);
        hud(31, 0, buf, seal < 40 ? PAL_ALERT : PAL_GOOD);

        hud(2, 1, "LEFT", post_ < 0 ? PAL_AMBER : PAL_HUD);
        hud(17, 1, "BAR", post_ == 0 ? PAL_AMBER : PAL_HUD);
        hud(31, 1, "RIGHT", post_ > 0 ? PAL_AMBER : PAL_HUD);

        int arm = int(std::lround(stam_ * 8.f));
        arm = std::clamp(arm, 0, 8);
        char marks[9];
        for (int i = 0; i < 8; ++i) marks[i] = i < arm ? '#' : '.';
        marks[8] = 0;
        std::snprintf(buf, sizeof buf, "ARM %s", marks);
        hud(1, 2, buf, stamLock_ ? PAL_ALERT : PAL_HUD);

        char flags[40] = "";
        if (holding_) std::strncat(flags, "HOLD ", sizeof flags - 1);
        if (gustLeft_ > 0) std::strncat(flags, "GUST ", sizeof flags - 1);
        if (wedgeOn_ >= 0) std::strncat(flags, "WEDGE", sizeof flags - 1);
        if (flags[0]) hud(22, 2, flags, gustLeft_ > 0 ? PAL_ALERT : PAL_GOOD);

        int hot = soonest(-99);
        if (hot >= 0) {
            float eta = foes_[hot].z / std::max(foes_[hot].speed, 0.1f);
            if (eta < 4.2f) {
                std::snprintf(buf, sizeof buf, "THEY HIT %s", postName(foes_[hot].lane));
                hudC(26, buf, eta < 1.6f ? PAL_ALERT : PAL_AMBER);
            } else {
                hudC(26, "WATCH THE ROAD", PAL_HUD);
            }
        } else if (gustLeft_ > 0) {
            hudC(26, "WIND ON THE DOOR", PAL_ALERT);
        } else if (age_ < 8 * 60) {
            hudC(26, "STAND ON THE POST AND HOLD", PAL_HUD);
        }
    }
}

}  // namespace rdoor

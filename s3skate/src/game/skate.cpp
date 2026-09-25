#include "game/skate.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace skate {
namespace {

constexpr float kSpeed = 2.5f;
constexpr float kGrav = 0.18f;
constexpr float kOllie = 4.8f;
constexpr float kPopV = 2.6f;
constexpr float kTorque = 4.2f;
constexpr float kLandAng = 15.f;
constexpr float kRailAng = 12.f;
constexpr float kManAng = 12.f;
constexpr float kCatchAng = 16.f;
constexpr float kRailX0 = 668.f;
constexpr float kRailX1 = 870.f;
constexpr float kRailY = 20.f;
constexpr float kPopX = 848.f;
constexpr float kMan0 = 1260.f;
constexpr float kMan1 = 1410.f;
constexpr float kGround = 168.f;
constexpr float kAnchor = 112.f;
constexpr int kPts[Game::kTricks] = {100, 200, 350, 400, 250, 400};

struct Deck {
    float x0, x1, top;
};

const Deck kDecks[] = {
    {0.f, 210.f, 0.f},
    {280.f, 460.f, 0.f},
    {488.f, 650.f, 26.f},
    {890.f, 1060.f, 0.f},
    {1148.f, 1465.f, 0.f},
    {1540.f, 1900.f, 0.f},
};

const char* kName[Game::kTricks] = {"CURB", "LEDGE", "RAIL", "STAIRS", "MANUAL", "BANK"};

float clampf(float v, float a, float b) {
    if (v < a) return a;
    if (v > b) return b;
    return v;
}

}  // namespace

int Game::line() const {
    int n = 0;
    for (int i = 0; i < kTricks; i++)
        if (did_[i]) n++;
    return n;
}

int Game::deckAt(float x) const {
    int found = -1;
    for (int i = 0; i < 6; i++) {
        if (x >= kDecks[i].x0 && x < kDecks[i].x1) found = i;
    }
    return found;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    score_ = 0;
    falls_ = 0;
    won_ = false;
    over_ = false;
    mode_ = Mode::Title;
    hold_ = 0;
    tick_ = 0;
    t_ = 0;
    x_ = 50.f;
    y_ = 0;
    vy_ = 0;
    angle_ = 0;
    grounded_ = true;
    grinding_ = false;
    grinded_ = false;
    manual_ = false;
    grindFrames_ = 0;
    for (int i = 0; i < kTricks; i++) did_[i] = false;
    fail_ = "";
    say_[0] = 0;
    sayT_ = 0;
    fanStep_ = -1;
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
}

void Game::beginRun(bool keepFalls) {
    if (!keepFalls) falls_ = 0;
    score_ = 0;
    won_ = false;
    over_ = false;
    x_ = 40.f;
    y_ = 0;
    vy_ = 0;
    angle_ = 0;
    grounded_ = true;
    grinding_ = false;
    grinded_ = false;
    manual_ = false;
    grindFrames_ = 0;
    shake_ = 0;
    for (int i = 0; i < kTricks; i++) did_[i] = false;
    fail_ = "";
    mode_ = Mode::Run;
    std::snprintf(say_, sizeof say_, "DROP IN");
    sayT_ = 0.7f;
    blip(330.f, 0.08f, 5);
}

void Game::blip(float freq, float vol, int frames) {
    if (!sys_ || fanStep_ >= 0) return;
    sys_->apu.tone(0, freq, vol);
    blip_ = frames;
}

void Game::bail(const char* why) {
    fail_ = why ? why : "FALL";
    score_ = 0;
    for (int i = 0; i < kTricks; i++) did_[i] = false;
    falls_++;
    grounded_ = false;
    grinding_ = false;
    grinded_ = false;
    manual_ = false;
    vy_ = 0;
    shake_ = 8;
    fallT_ = 48;
    mode_ = Mode::Fall;
    won_ = false;
    std::snprintf(say_, sizeof say_, "FALL");
    sayT_ = 1.2f;
    if (sys_) {
        sys_->apu.noiseBurst(0.28f, 900.f, 0.18f);
        sys_->rumble(0.7f, 0.3f, 160);
        sys_->setLight(180, 30, 20);
    }
    if (bot_) over_ = true;
}

void Game::scoreTrick(int i) {
    if (i < 0 || i >= kTricks || did_[i]) return;
    did_[i] = true;
    score_ += kPts[i];
    std::snprintf(say_, sizeof say_, "%s", kName[i]);
    sayT_ = 0.85f;
    blip(420.f + float(i) * 55.f, 0.1f, 7);
    if (sys_) sys_->rumble(0.15f, 0.25f, 70);
}

bool Game::botTap() const {
    if (!grounded_ || grinding_ || manual_) return false;
    if (!did_[0] && x_ >= 175.f && x_ < 210.f) return true;
    if (did_[0] && !did_[1] && x_ >= 420.f && x_ < 460.f) return true;
    if (did_[1] && !did_[2] && y_ > 18.f && x_ >= 620.f && x_ < 650.f) return true;
    if (did_[2] && !did_[3] && x_ >= 1045.f && x_ < 1060.f) return true;
    if (did_[3] && did_[4] && !did_[5] && x_ >= 1445.f && x_ < 1465.f) return true;
    return false;
}

float Game::stick() const {
    bool balance = grinding_ || manual_ || !grounded_;
    if (bot_) {
        if (!balance) return 0;
        if (angle_ > 1.6f) return -1.f;
        if (angle_ < -1.6f) return 1.f;
        return 0;
    }
    if (!sys_) return 0;
    const gs::Pad& pad = sys_->pad;
    if (std::fabs(pad.axisX) > 0.12f) return clampf(pad.axisX, -1.f, 1.f);
    float s = 0;
    if (pad.down(gs::BTN_LEFT)) s -= 1.f;
    if (pad.down(gs::BTN_RIGHT)) s += 1.f;
    return s;
}

void Game::grindStep() {
    x_ += kSpeed;
    y_ = kRailY;
    vy_ = 0;
    grindFrames_++;
    if (grindFrames_ >= 4) grinded_ = true;
    if (x_ >= kPopX) {
        grinding_ = false;
        grounded_ = false;
        vy_ = kPopV;
        y_ = kRailY;
        blip(640.f, 0.07f, 4);
    }
}

void Game::groundStep() {
    x_ += kSpeed;
    int d = deckAt(x_);
    if (d < 0) {
        grounded_ = false;
        manual_ = false;
        vy_ = 0;
        return;
    }
    y_ = kDecks[d].top;
    vy_ = 0;
    if (manual_) {
        if (x_ >= kMan1) {
            manual_ = false;
            angle_ = 0;
            scoreTrick(4);
        }
        return;
    }
    if (d == 4 && did_[3] && !did_[4] && x_ >= kMan0 && x_ < kMan1) manual_ = true;
}

void Game::airStep() {
    float prevY = y_;
    vy_ -= kGrav;
    y_ += vy_;
    x_ += kSpeed;

    if (!did_[2] && x_ >= kRailX0 && x_ < kRailX1 && vy_ < 0 && prevY > kRailY && y_ <= kRailY && y_ >= kRailY - 16.f) {
        if (std::fabs(angle_) <= kCatchAng) {
            grinding_ = true;
            grounded_ = false;
            y_ = kRailY;
            vy_ = 0;
            grindFrames_ = 1;
            blip(520.f, 0.08f, 5);
            return;
        }
    }

    int d = deckAt(x_);
    if (d >= 0 && prevY > kDecks[d].top && y_ <= kDecks[d].top && vy_ <= 0) {
        if (std::fabs(angle_) > kLandAng) {
            bail("CROOKED");
            return;
        }
        y_ = kDecks[d].top;
        vy_ = 0;
        grounded_ = true;
        angle_ = 0;
        shake_ = std::max(shake_, 3);
        if (d == 1) scoreTrick(0);
        else if (d == 2) scoreTrick(1);
        else if (d == 3) {
            if (!did_[2]) {
                if (!grinded_) {
                    bail("MISSED THE RAIL");
                    return;
                }
                scoreTrick(2);
                grinded_ = false;
            }
        } else if (d == 4) {
            scoreTrick(3);
            if (did_[3] && !did_[4] && x_ >= kMan0 && x_ < kMan1) manual_ = true;
        } else if (d == 5 && !did_[5]) {
            if (!(did_[0] && did_[1] && did_[2] && did_[3] && did_[4])) {
                bail("BROKEN LINE");
                return;
            }
            scoreTrick(5);
            won_ = true;
            over_ = true;
            mode_ = Mode::Win;
            fanStep_ = 0;
            fanT_ = 0;
            sayT_ = 2.f;
            std::snprintf(say_, sizeof say_, "LINE LANDED");
            if (sys_) {
                sys_->rumble(0.25f, 0.55f, 200);
                sys_->setLight(40, 180, 90);
            }
        }
        return;
    }

    if (d >= 0 && y_ < kDecks[d].top - 12.f && vy_ < 0.f) bail("PIT");
    else if (d < 0 && y_ <= 0.f) bail("PIT");
    else if (y_ < -36.f) bail("PIT");
}

void Game::physics() {
    bool tap = false;
    bool steady = false;
    if (bot_) {
        tap = botTap();
    } else if (sys_) {
        const gs::Pad& pad = sys_->pad;
        tap = pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_UP);
        steady = pad.down(gs::BTN_B);
    }
    float s = stick();

    if (tap && grounded_ && !grinding_ && !manual_) {
        grounded_ = false;
        vy_ = kOllie;
        blip(740.f, 0.09f, 5);
    }

    bool balance = grinding_ || manual_ || !grounded_;
    if (!balance) {
        angle_ = 0;
    } else {
        float drift = 0.42f;
        if (grinding_) drift = 0.48f + 0.35f * std::sin(x_ * 0.11f + 0.2f);
        else if (manual_) drift = 0.40f + 0.45f * std::sin(x_ * 0.10f + 0.5f);
        angle_ += s * kTorque + drift;
        if (steady) angle_ *= 0.86f;
        angle_ = clampf(angle_, -80.f, 80.f);
    }

    if (manual_ && std::fabs(angle_) > kManAng) {
        bail("MANUAL");
        return;
    }
    if (grinding_ && std::fabs(angle_) > kRailAng) {
        bail("RAIL");
        return;
    }

    if (grinding_) grindStep();
    else if (grounded_) groundStep();
    else airStep();
}

void Game::audio() {
    if (!sys_) return;
    gs::APU& a = sys_->apu;
    if (mode_ == Mode::Run && grounded_ && !manual_) a.noise(0.018f, 500.f, true);
    else if (mode_ == Mode::Run && grinding_) a.noise(0.04f, 2200.f, true);
    else a.noise(0.f, 400.f, false);

    if (fanStep_ >= 0) {
        fanT_++;
        if (fanT_ == 1 || fanT_ == 10 || fanT_ == 18 || fanT_ == 28) {
            static const float n[4] = {392.f, 523.f, 659.f, 784.f};
            int i = fanT_ < 10 ? 0 : fanT_ < 18 ? 1 : fanT_ < 28 ? 2 : 3;
            a.tone(0, n[i], 0.14f);
            a.tone(1, n[i] * 0.5f, 0.05f);
        }
        if (fanT_ > 56) {
            a.tone(0, 0, 0);
            a.tone(1, 0, 0);
            fanStep_ = -1;
        }
        return;
    }
    if (manual_ && mode_ == Mode::Run) a.tone(1, 196.f, 0.03f);
    else if (blip_ == 0) a.tone(1, 0, 0);

    if (blip_ > 0) {
        blip_--;
        if (blip_ == 0) a.tone(0, 0, 0);
    }
}

const char* Game::coach() const {
    if (manual_) {
        if (angle_ > 7.f) return "MANUAL  LEFT";
        if (angle_ < -7.f) return "MANUAL  RIGHT";
        return "HOLD THE MANUAL";
    }
    if (grinding_ || !grounded_) {
        if (grinding_ && std::fabs(angle_) <= 7.f) return "LEVEL THE RAIL";
        if (angle_ > 7.f) return "HOLD LEFT";
        if (angle_ < -7.f) return "HOLD RIGHT";
        return "LEVEL TO LAND";
    }
    if (!did_[0] && x_ > 110.f) return "OLLIE THE CURB";
    if (did_[0] && !did_[1] && x_ > 320.f) return "OLLIE THE LEDGE";
    if (did_[1] && !did_[2] && x_ > 500.f) return "POP THE RAIL";
    if (did_[2] && !did_[3] && x_ > 960.f) return "OLLIE THE STAIRS";
    if (did_[3] && !did_[4] && x_ > 1180.f) return "MANUAL NEXT";
    if (did_[4] && !did_[5] && x_ > 1412.f) return "OLLIE THE BANK";
    return "RIDE THE LINE";
}

float Game::screenX(float wx) const { return kAnchor + (wx - camX_); }

float Game::screenY(float wy) const { return kGround - wy; }

void Game::image(const gs::Mipped& m, float left, float top, float h, int pal, bool flip, int fog, bool shadow) {
    if (!sys_ || h < 1.5f || h > 420.f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    if (left > 340.f || top > 250.f || left + w < -30.f || top + h < -30.f) return;
    gs::Sprite s;
    auto q = [](float v) { return int16_t(std::lround(clampf(v, -400.f, 800.f))); };
    s.x = q(left);
    s.y = q(top);
    s.w = int16_t(std::max(1, int(std::lround(w))));
    s.h = int16_t(std::max(1, int(std::lround(h))));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float bottom, float h, int pal, bool flip, int fog) {
    if (m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    image(m, cx - w * 0.5f, bottom - h, h, pal, flip, fog, false);
}

void Game::backdrop() {
    gs::VDP& v = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        v.road[y].on = false;
        v.lineFog[y] = 0;
        if (y < 108) {
            float u = float(y) / 108.f;
            int r = 8 + int((1.f - u) * 4.f);
            int g = 3 + int(u * 4.f);
            int b = 10 - int(u * 4.f);
            v.lineBackdrop[y] = gs::rgb4(r, g, std::max(3, b));
        } else if (y < int(kGround)) {
            float u = float(y - 108) / (kGround - 108.f);
            int r = 12 - int(u * 5.f);
            int g = 6 - int(u * 2.f);
            int b = 5 - int(u * 2.f);
            v.lineBackdrop[y] = gs::rgb4(r, g, b);
        } else {
            v.lineBackdrop[y] = gs::rgb4(2, 1, 3);
        }
    }
}

void Game::world() {
    auto strip = [&](const gs::Mipped& m, float x0, float x1, float top, float h, int pal) {
        if (m.h < 1) return;
        float w = h * float(m.w) / float(m.h);
        float step = std::max(8.f, w * 0.86f);
        for (float x = x0; x < x1; x += step) {
            float sx = screenX(x);
            if (sx > 340.f || sx + w < -20.f) continue;
            image(m, sx, screenY(top), h, pal, false, 0, false);
        }
    };

    // First sprites sit in front. Cones, then decks, then the skyline, then the sun.
    spr(art_.cone, screenX(198.f), screenY(0), 18.f, PAL_PROP, false, 0);
    spr(art_.cone, screenX(452.f), screenY(0), 18.f, PAL_PROP, false, 0);
    spr(art_.cone, screenX(640.f), screenY(26.f), 18.f, PAL_PROP, false, 0);
    spr(art_.cone, screenX(1048.f), screenY(0), 18.f, PAL_PROP, false, 0);
    spr(art_.cone, screenX(1454.f), screenY(0), 18.f, PAL_PROP, false, 0);
    spr(art_.can, screenX(120.f), screenY(0), 14.f, PAL_PROP, false, 0);
    spr(art_.can, screenX(980.f), screenY(0), 14.f, PAL_PROP, true, 0);

    strip(art_.deck, kDecks[0].x0, kDecks[0].x1, 0, 26.f, PAL_DECK);
    strip(art_.deck, kDecks[1].x0, kDecks[1].x1, 0, 26.f, PAL_DECK);
    for (float x = kDecks[2].x0; x < kDecks[2].x1; x += 26.f)
        spr(art_.block, screenX(x + 12.f), screenY(0) + 2.f, 34.f, PAL_DECK, false, 0);
    strip(art_.deck, kDecks[2].x0, kDecks[2].x1, 26.f, 22.f, PAL_DECK);
    strip(art_.rail, kRailX0, 860.f, kRailY, 22.f, PAL_RAIL);
    strip(art_.deck, kDecks[3].x0, kDecks[3].x1, 0, 26.f, PAL_DECK);
    strip(art_.stair, 1064.f, 1140.f, -6.f, 32.f, PAL_PROP);
    strip(art_.deck, kDecks[4].x0, kMan0, 0, 26.f, PAL_DECK);
    strip(art_.pad, kMan0, kMan1, 0, 26.f, PAL_PAD);
    strip(art_.deck, kMan1, kDecks[4].x1, 0, 26.f, PAL_DECK);
    image(art_.bank, screenX(1468.f), screenY(0) - 6.f, 26.f, PAL_DECK, false, 0, false);
    image(art_.bank, screenX(1532.f), screenY(0) - 6.f, 26.f, PAL_DECK, true, 0, false);
    strip(art_.deck, kDecks[5].x0, kDecks[5].x1, 0, 26.f, PAL_DECK);

    struct Far {
        float x;
        float h;
        bool tower;
    };
    static const Far far[] = {{40, 78, true},  {180, 64, false}, {340, 86, true}, {520, 60, false},
                              {760, 74, true}, {980, 66, false}, {1240, 90, true}, {1500, 62, false},
                              {1720, 80, true}};
    for (const Far& f : far) {
        float sx = kAnchor + (f.x - camX_) * 0.38f;
        const gs::Mipped& m = f.tower ? art_.tower : art_.building;
        spr(m, sx, 112.f, f.h, PAL_CITY, false, 6);
    }

    const float jig = shake_ ? ((tick_ & 1) ? 1.f : -1.f) : 0.f;
    float drift = std::fmod(t_ * 14.f, 380.f);
    spr(art_.cloud, drift - 30.f, 36.f, 16.f, PAL_FX, false, 2);
    spr(art_.cloud, std::fmod(drift * 0.6f + 160.f, 400.f) - 40.f, 28.f, 20.f, PAL_FX, true, 3);
    spr(art_.sun, 286.f, 42.f + jig, 30.f, PAL_FX, false, 0);
}

void Game::rider() {
    float bob = (mode_ == Mode::Title) ? std::sin(t_ * 2.4f) * 2.f : 0.f;
    float wx = (mode_ == Mode::Title) ? 50.f : x_;
    float wy = (mode_ == Mode::Title) ? 0.f : y_;
    if (mode_ == Mode::Title) {
        int phase = (tick_ / 36) % 3;
        if (phase == 1) wy = 18.f + bob;
        if (phase == 2) wy = 8.f;
    }
    float sx = screenX(wx);
    float jig = shake_ ? ((tick_ & 1) ? float(shake_) * 0.4f : -float(shake_) * 0.4f) : 0.f;
    sx += jig;
    float feet = screenY(wy) + 2.f;
    image(art_.shadow, sx - 16.f, feet - 4.f, 10.f, PAL_FX, false, 0, true);

    const gs::Mipped* body = &art_.ride;
    float h = 58.f;
    if (mode_ == Mode::Fall) {
        body = &art_.bail;
        h = 32.f;
        feet += 4.f;
    } else if (mode_ == Mode::Title) {
        int phase = (tick_ / 36) % 3;
        if (phase == 1) {
            body = &art_.air;
            h = 54.f;
        } else if (phase == 2) {
            body = &art_.noseUp;
            h = 56.f;
        }
    } else if (manual_) {
        body = std::fabs(angle_) > 8.f && angle_ < 0 ? &art_.noseDown : &art_.noseUp;
        h = 56.f;
    } else if (grinding_) {
        body = &art_.grind;
        h = 50.f;
    } else if (!grounded_ && mode_ == Mode::Run) {
        if (angle_ > 8.f) body = &art_.noseUp;
        else if (angle_ < -8.f) body = &art_.noseDown;
        else body = &art_.air;
        h = 54.f;
    }
    spr(*body, sx, feet, h, PAL_SKATER, false, 0);

    if (grinding_ && (tick_ & 2)) {
        spr(art_.spark, sx - 10.f, feet + 2.f, 10.f, PAL_FX, false, 0);
        spr(art_.spark, sx + 12.f, feet + 1.f, 8.f, PAL_FX, true, 0);
    } else if (mode_ == Mode::Run && grounded_ && !manual_ && (tick_ & 4)) {
        spr(art_.dust, sx - 18.f, feet + 1.f, 8.f, PAL_FX, tick_ & 8, 0);
    }
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (!sys_ || row < 0 || row > 27) return;
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

void Game::hudC(int row, const std::string& s, int pal) { hud(20 - int(s.size()) / 2, row, s, pal); }

void Game::draw() {
    if (!sys_) return;
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    backdrop();
    // Sprites drawn first sit on top.
    rider();
    world();

    char buf[48];
    if (mode_ == Mode::Title) {
        spr(art_.logo, 160.f, 46.f, 28.f, PAL_GOLD, false, 0);
        hudC(7, "LAND THE LINE", PAL_GOLD);
        hudC(9, "A FALL ZEROS THE RUN", PAL_HUD);
        hudC(12, "ARROWS LEVEL THE BOARD", PAL_HUD);
        hudC(13, "C OLLIES   X STEADIES", PAL_HUD);
        hudC(16, "ENTER DROPS IN", PAL_GREEN);
        return;
    }

    hud(1, 0, "S3 SKATE", PAL_GOLD);
    std::snprintf(buf, sizeof buf, "RUN %d", score_);
    hud(39 - int(std::strlen(buf)), 0, buf, score_ > 0 ? PAL_GOLD : PAL_RED);

    char pips[8];
    for (int i = 0; i < kTricks; i++) pips[i] = did_[i] ? '#' : '-';
    pips[kTricks] = 0;
    std::snprintf(buf, sizeof buf, "LINE %d/%d  %s", line(), kTricks, pips);
    hud(1, 1, buf, line() == kTricks ? PAL_GREEN : PAL_HUD);
    std::snprintf(buf, sizeof buf, "FALLS %d", falls_);
    hud(39 - int(std::strlen(buf)), 1, buf, falls_ ? PAL_RED : PAL_GREEN);

    if (mode_ == Mode::Pause) {
        hudC(4, "PAUSED", PAL_GOLD);
        hudC(6, "ENTER RESUMES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Win) {
        hudC(3, "LINE LANDED", PAL_GREEN);
        hudC(5, "THE RUN HOLDS", PAL_GOLD);
        std::snprintf(buf, sizeof buf, "RUN %d   FALLS %d", score_, falls_);
        hudC(7, buf, PAL_HUD);
        if (!bot_) hudC(9, "ENTER RETRIES", PAL_HUD);
        return;
    }
    if (mode_ == Mode::Fall) {
        hudC(3, "FALL", PAL_RED);
        hudC(5, "THE RUN IS ZERO", PAL_GOLD);
        if (!bot_) hudC(7, "GET UP", PAL_HUD);
        return;
    }

    if (sayT_ > 0 && say_[0]) hudC(3, say_, PAL_GOLD);
    else hudC(3, coach(), PAL_HUD);

    bool balancing = manual_ || grinding_ || !grounded_;
    if (balancing) {
        int tilt = int(std::lround(clampf(angle_ / 3.f, -8.f, 8.f)));
        char meter[18];
        for (int i = 0; i < 17; i++) meter[i] = '-';
        meter[8] = '+';
        meter[8 + tilt] = 'O';
        meter[17] = 0;
        int mp = std::fabs(angle_) > 10.f ? PAL_RED : PAL_GREEN;
        hud(1, 4, "DN", PAL_HUD);
        hud(11, 4, meter, mp);
        hud(30, 4, "UP", PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += 1.f / 60.f;
    tick_++;
    if (sayT_ > 0) sayT_ = std::max(0.f, sayT_ - 1.f / 60.f);
    if (shake_ > 0) shake_--;

    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        hold_++;
        camX_ = 50.f;
        bool go = bot_ ? hold_ >= 30 : (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C));
        if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        if (go) beginRun(false);
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Pause) {
        camX_ = x_;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A))) mode_ = Mode::Run;
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Fall) {
        camX_ = x_;
        if (!bot_) {
            fallT_--;
            if (fallT_ <= 0 || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C)) beginRun(true);
        }
        draw();
        audio();
        return;
    }
    if (mode_ == Mode::Win) {
        camX_ = x_;
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_C))) beginRun(false);
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) sys.quit();
        draw();
        audio();
        return;
    }

    if (!bot_ && pad.pressed(gs::BTN_START)) {
        mode_ = Mode::Pause;
        draw();
        audio();
        return;
    }
    if (!bot_ && pad.pressed(gs::BTN_MODE)) {
        sys.quit();
        return;
    }

    camX_ = x_;
    physics();
    if (mode_ == Mode::Run) camX_ = x_;
    draw();
    audio();
}

}  // namespace skate

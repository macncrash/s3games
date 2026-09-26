#include "game/door.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace depotdoor {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr int HOLD = 180 * 60;
constexpr int DOG = 1;
constexpr int TRUCK = 2;

constexpr float DOOR_L = 96.f;
constexpr float DOOR_R = 224.f;
constexpr float DOOR_TOP = 68.f;
constexpr float DOOR_BOT = 184.f;
constexpr float DOOR_W = DOOR_R - DOOR_L;
constexpr int SLATS = 9;

uint16_t mixC(uint16_t a, uint16_t b, float t) {
    t = std::clamp(t, 0.f, 1.f);
    auto ch = [](uint16_t c, int s) { return (c >> s) & 15; };
    auto L = [&](int s) { return int(std::lround(ch(a, s) + (ch(b, s) - ch(a, s)) * t)); };
    return gs::rgb4(L(8), L(4), L(0));
}

gs::FMPatch chainPatch() {
    gs::FMPatch p;
    p.alg = 5;
    p.fb = 0.28f;
    p.op[0] = {1.f, 0.65f, 0.04f, 0.35f, 0.7f, 0.4f};
    p.op[1] = {2.f, 0.25f, 0.08f, 0.4f, 0.45f, 0.35f};
    p.op[2] = {0.5f, 0.35f, 0.1f, 0.4f, 0.6f, 0.3f};
    p.op[3] = {1.5f, 0.18f, 0.06f, 0.3f, 0.35f, 0.25f};
    p.vol = 0.04f;
    p.drive = 0.15f;
    p.tone = 380.f;
    p.glide = 0.02f;
    return p;
}

float doorBottom(float lift) {
    float travel = DOOR_BOT - DOOR_TOP - 18.f;
    return DOOR_BOT - std::clamp(lift, 0.f, 1.f) * travel;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Won) return 2;
    if (mode_ == Mode::Lost) return 3;
    if (mode_ == Mode::Play || mode_ == Mode::Pause) return 1;
    return 0;
}

void Game::blip(float freq) {
    sys_->apu.tone(1, freq, 0.055f);
    blip_ = 0.06f;
}

void Game::burst(float x, float y) {
    for (Spark& s : sparks_) {
        if (s.life > 0.f) continue;
        float ang = float(age_ % 11) * 0.57f;
        s.x = x;
        s.y = y;
        s.vx = std::sin(ang) * 40.f;
        s.vy = -28.f - float(age_ % 5) * 6.f;
        s.life = 0.38f;
        return;
    }
}

int Game::upcoming(int kind, int* side) const {
    for (int i = markNext_; i < markCount_; i++) {
        if (marks_[i].kind != kind) continue;
        if (side) *side = marks_[i].side;
        return marks_[i].frame - age_;
    }
    return -1;
}

void Game::schedule() {
    markCount_ = 0;
    markNext_ = 0;
    auto add = [&](int frame, int kind, int side) {
        if (markCount_ >= int(sizeof marks_ / sizeof marks_[0])) return;
        marks_[markCount_++] = {frame, kind, side};
    };
    for (int n = 0; n < 40; n++) {
        int dogF = 360 + n * 480;
        int truckF = dogF + 216;
        if (dogF >= HOLD - 80) break;
        add(dogF, DOG, n & 1);
        if (truckF < HOLD - 30) add(truckF, TRUCK, 1);
    }
    std::sort(marks_, marks_ + markCount_, [](const Mark& a, const Mark& b) {
        if (a.frame != b.frame) return a.frame < b.frame;
        if (a.kind != b.kind) return a.kind < b.kind;
        return a.side < b.side;
    });
}

void Game::begin() {
    age_ = 0;
    misses_ = 0;
    lift_ = 0.04f;
    arms_ = 1.f;
    slip_ = 0.f;
    sack_ = 0.f;
    sackCd_ = 2.f;
    dogL_ = 0.f;
    dogR_ = 0.f;
    dogVisL_ = 0.f;
    dogVisR_ = 0.f;
    shake_ = 0.f;
    recoil_ = 0.f;
    missT_ = 0.f;
    blip_ = 0.f;
    tick_ = 0.f;
    fanT_ = 0.f;
    fan_ = -1;
    dogLive_ = false;
    dogAnswered_ = false;
    truckLive_ = false;
    hauling_ = false;
    won_ = false;
    over_ = false;
    gate_ = 12;
    reason_ = "THE DOOR OPENED";
    for (Spark& s : sparks_) s.life = 0.f;
    schedule();
    mode_ = Mode::Play;
    sys_->apu.setPatch(0, chainPatch());
    sys_->apu.keyOn(0, 64.f, 0.02f);
    sys_->setLight(90, 58, 22);
}

void Game::toTitle() {
    mode_ = Mode::Title;
    lift_ = 0.f;
    arms_ = 1.f;
    slip_ = 0.f;
    sack_ = 0.f;
    dogL_ = 0.f;
    dogR_ = 0.f;
    dogLive_ = false;
    truckLive_ = false;
    hauling_ = false;
    shake_ = 0.f;
    recoil_ = 0.f;
    fan_ = -1;
    gate_ = 16;
    over_ = false;
    won_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    sys_->setLight(70, 46, 18);
}

void Game::finish(bool held) {
    mode_ = held ? Mode::Won : Mode::Lost;
    won_ = held;
    over_ = true;
    gate_ = 28;
    fan_ = held ? 0 : -1;
    fanT_ = 0.f;
    hauling_ = false;
    dogLive_ = false;
    truckLive_ = false;
    sys_->apu.keyOff(0);
    sys_->apu.tone(0, 0, 0);
    sys_->apu.tone(1, 0, 0);
    sys_->apu.tone(2, 0, 0);
    if (held) {
        lift_ = 0.f;
        dogL_ = 0.f;
        dogR_ = 0.f;
        reason_ = "THE DOOR HELD";
        sys_->rumble(0.25f, 0.55f, 260);
        sys_->setLight(40, 140, 64);
        sys_->apu.noiseBurst(0.25f, 90.f, 0.2f);
    } else {
        lift_ = 1.f;
        shake_ = 1.2f;
        reason_ = "THE DOOR OPENED";
        sys_->rumble(0.9f, 0.25f, 300);
        sys_->setLight(170, 24, 16);
        sys_->apu.noiseBurst(0.75f, 140.f, 0.4f);
    }
}

void Game::pressDog(int side) {
    if (side == 0) dogL_ = 0.f;
    else dogR_ = 0.f;
    if (dogLive_ && !dogAnswered_ && dogSide_ == side) {
        dogAnswered_ = true;
        lift_ = std::max(0.f, lift_ - 0.012f);
        blip(740.f);
        sys_->apu.noiseBurst(0.12f, 520.f, 0.04f);
        float x = side == 0 ? DOOR_L + 8.f : DOOR_R - 8.f;
        burst(x, DOOR_BOT - 6.f);
    }
}

void Game::trySack() {
    if (sack_ <= 0.f && sackCd_ <= 0.f && lift_ < 0.55f) {
        sack_ = 2.3f;
        sackCd_ = 9.5f;
        blip(230.f);
        sys_->apu.noiseBurst(0.18f, 260.f, 0.06f);
        burst(160.f, doorBottom(lift_) + 4.f);
    } else if (sack_ <= 0.f) {
        blip(80.f);
    }
}

void Game::botIntent(bool inTruck, int truckIn, bool& haul, bool& left, bool& right, bool& sack) {
    if (dogLive_ && !dogAnswered_) {
        if (dogSide_ == 0) left = true;
        else right = true;
    }
    if (dogL_ > 0.2f) left = true;
    if (dogR_ > 0.2f) right = true;

    bool brace = inTruck && truckIn <= 18;
    if (slip_ > 0.f) haul = false;
    else if (brace) haul = true;
    else if (arms_ < 0.22f) haul = false;
    else if (lift_ > 0.10f) haul = true;

    if (!dogLive_ && !inTruck && sack_ <= 0.f && sackCd_ <= 0.f && arms_ < 0.30f && lift_ > 0.06f && lift_ < 0.36f)
        sack = true;
}

void Game::humanIntent(bool& haul, bool& left, bool& right, bool& sack) {
    const gs::Pad& p = sys_->pad;
    haul = p.down(gs::BTN_C) || p.down(gs::BTN_TURBO) || p.down(gs::BTN_DOWN) || p.accel > 0.45f;
    left = p.pressed(gs::BTN_LEFT);
    right = p.pressed(gs::BTN_RIGHT);
    sack = p.pressed(gs::BTN_B) || p.pressed(gs::BTN_A) || p.pressed(gs::BTN_Y);
}

void Game::update() {
    int dogSide = 0;
    int dogIn = upcoming(DOG, &dogSide);
    int truckIn = upcoming(TRUCK, nullptr);
    int dogTele = age_ > HOLD - 45 * 60 ? 46 : 68;
    int truckTele = age_ > HOLD - 45 * 60 ? 50 : 72;
    bool inDog = dogIn >= 0 && dogIn <= dogTele;
    bool inTruck = truckIn >= 0 && truckIn <= truckTele;

    if (inDog) {
        if (!dogLive_) {
            dogLive_ = true;
            dogSide_ = dogSide;
            dogAnswered_ = false;
            blip(820.f);
        }
    } else {
        dogLive_ = false;
    }
    if (inTruck) {
        if (!truckLive_) {
            truckLive_ = true;
            blip(180.f);
        }
    } else {
        truckLive_ = false;
    }

    bool left = false, right = false, sack = false, haul = false;
    if (bot_) botIntent(inTruck, truckIn, haul, left, right, sack);
    else humanIntent(haul, left, right, sack);
    if (left) pressDog(0);
    if (right) pressDog(1);
    if (sack) trySack();

    bool truckHit = false;
    while (markNext_ < markCount_ && marks_[markNext_].frame <= age_) {
        Mark m = marks_[markNext_++];
        if (m.kind == DOG) {
            if (!(dogAnswered_ && dogSide_ == m.side)) {
                if (m.side == 0) dogL_ = 1.f;
                else dogR_ = 1.f;
                lift_ += 0.12f;
                shake_ = std::max(shake_, 0.85f);
                missT_ = 0.75f;
                missSide_ = m.side;
                misses_++;
                blip(110.f);
                sys_->apu.noiseBurst(0.38f, 180.f, 0.1f);
                burst(m.side == 0 ? DOOR_L : DOOR_R, DOOR_BOT - 10.f);
            }
            dogLive_ = false;
            dogAnswered_ = false;
        } else {
            truckHit = true;
            truckLive_ = false;
        }
    }

    float siege = std::clamp(age_ / float(HOLD), 0.f, 1.f);
    if (truckHit) {
        float add = 0.15f + 0.05f * siege;
        bool braced = haul && slip_ <= 0.f && arms_ > 0.02f;
        if (braced) add = 0.032f + 0.018f * siege;
        else misses_++;
        if (sack_ > 0.f) add *= 0.4f;
        if (dogL_ > 0.5f) add += 0.06f;
        if (dogR_ > 0.5f) add += 0.06f;
        lift_ += add;
        shake_ = 1.f;
        recoil_ = 0.32f;
        sys_->apu.noiseBurst(braced ? 0.28f : 0.55f, braced ? 240.f : 130.f, 0.14f);
        sys_->rumble(braced ? 0.35f : 0.8f, 0.2f, 80);
        burst(DOOR_R - 16.f, 150.f);
    }

    if (slip_ > 0.f) haul = false;
    hauling_ = haul && arms_ > 0.f && slip_ <= 0.f;

    float creep = (0.046f + 0.030f * siege) * (1.f + 0.95f * dogL_ + 0.95f * dogR_);
    if (sack_ > 0.f) creep *= 0.10f;
    lift_ += creep * DT;
    if (hauling_) {
        lift_ -= 0.27f * DT;
        arms_ -= 0.105f * DT;
        if (arms_ <= 0.f) {
            arms_ = 0.f;
            slip_ = 0.62f;
            hauling_ = false;
            lift_ += 0.07f;
            shake_ = std::max(shake_, 0.7f);
            blip(90.f);
            sys_->apu.noiseBurst(0.3f, 160.f, 0.08f);
        }
    } else {
        float rec = slip_ > 0.f ? 0.42f : 0.26f;
        if (sack_ > 0.f) rec += 0.1f;
        arms_ = std::min(1.f, arms_ + rec * DT);
    }

    if (slip_ > 0.f) slip_ = std::max(0.f, slip_ - DT);
    if (sack_ > 0.f) sack_ = std::max(0.f, sack_ - DT);
    if (sackCd_ > 0.f) sackCd_ = std::max(0.f, sackCd_ - DT);
    if (missT_ > 0.f) missT_ = std::max(0.f, missT_ - DT);
    if (recoil_ > 0.f) recoil_ = std::max(0.f, recoil_ - DT);
    dogVisL_ += (dogL_ - dogVisL_) * 0.25f;
    dogVisR_ += (dogR_ - dogVisR_) * 0.25f;
    for (Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        s.life -= DT;
        s.x += s.vx * DT;
        s.y += s.vy * DT;
        s.vy += 70.f * DT;
    }

    float hz = hauling_ ? 78.f + lift_ * 90.f : 52.f;
    float vol = hauling_ ? 0.045f + lift_ * 0.03f : 0.012f;
    sys_->apu.setFreq(0, hz);
    sys_->apu.setVol(0, vol);

    if (age_ > 0 && age_ % 60 == 0) {
        bool late = age_ >= HOLD - 600;
        sys_->apu.tone(2, late ? 720.f : 330.f, late ? 0.05f : 0.03f);
        tick_ = 0.045f;
    }

    lift_ = std::max(0.f, lift_);
    age_++;
    if (age_ >= HOLD) {
        finish(true);
        return;
    }
    if (lift_ >= 1.f) {
        lift_ = 1.f;
        finish(false);
    }
}

void Game::fanfare() {
    if (fan_ < 0) return;
    fanT_ += DT;
    if (fan_ < 4 && fanT_ >= 0.16f) {
        static const float notes[] = {349.f, 440.f, 523.f, 698.f};
        sys_->apu.tone(1, notes[fan_], 0.07f);
        fan_++;
        fanT_ = 0.f;
    } else if (fan_ >= 4 && fanT_ > 0.5f) {
        sys_->apu.tone(1, 0, 0);
        fan_ = -1;
    }
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= DT;
        if (blip_ <= 0.f) sys_->apu.tone(1, 0, 0);
    }
    if (tick_ > 0.f) {
        tick_ -= DT;
        if (tick_ <= 0.f) sys_->apu.tone(2, 0, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = false;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 4));
    mode_ = Mode::Title;
    won_ = false;
    over_ = false;
    gate_ = 18;
    sys.setLight(70, 46, 18);
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    if (gate_ > 0) gate_--;
    if (shake_ > 0.f) shake_ = std::max(0.f, shake_ - 0.035f);
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        if (gate_ == 0 && start) begin();
        else if (gate_ == 0 && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Play) {
        if (!bot_ && gate_ == 0 && start) {
            mode_ = Mode::Pause;
            sys.apu.keyOff(0);
        } else if (!bot_ && gate_ == 0 && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) {
                sys.eject();
                return;
            }
            toTitle();
        } else {
            update();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            gate_ = 8;
            sys.apu.setPatch(0, chainPatch());
            sys.apu.keyOn(0, 64.f, 0.02f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            toTitle();
        }
    } else {
        if (mode_ == Mode::Won) fanfare();
        if (gate_ == 0 && start) begin();
        else if (gate_ == 0 && pad.pressed(gs::BTN_MODE)) toTitle();
    }
    serviceAudio();
    draw();
}

void Game::hud(int col, int row, const char* s, int pal) {
    if (!s || row < 0 || row > 27) return;
    for (int i = 0; s[i]; i++) {
        int x = col + i;
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::hudC(int row, const char* s, int pal) {
    if (!s) return;
    hud(20 - int(std::strlen(s)) / 2, row, s, pal);
}

void Game::text(const char* s, float x, float y, float scale, int pal) {
    if (!s) return;
    float width = 0.f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) width += 10.f * scale;
        else width += float(art_.glyph[c - 32].w) * scale;
    }
    x -= width * 0.5f;
    for (const char* p = s; *p; ++p) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c <= 32 || c >= 128) {
            x += 10.f * scale;
            continue;
        }
        const gs::Mipped& g = art_.glyph[c - 32];
        float gw = float(g.w) * scale;
        float gh = float(g.h) * scale;
        spr(g, x + gw * 0.5f, y, gh, pal);
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
    if (s.x > gs::SCREEN_W + 64 || s.x + s.w < -64 || s.y > gs::SCREEN_H + 64 || s.y + s.h < -64) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::sky(float shx) {
    gs::VDP& v = sys_->vdp;
    float warm = mode_ == Mode::Won ? 0.4f : 0.f;
    float hot = mode_ == Mode::Lost ? 0.5f : std::clamp(lift_, 0.f, 1.f) * 0.22f;
    float flick = 0.5f + 0.5f * std::sin(float(sys_->frame) * 0.17f);
    uint16_t top = gs::rgb4(1, 1, 3);
    uint16_t mid = gs::rgb4(2, 2, 5);
    uint16_t low = gs::rgb4(5, 3, 3);
    low = mixC(low, gs::rgb4(8, 5, 2), 0.35f + flick * 0.15f);
    if (warm > 0.f) {
        top = mixC(top, gs::rgb4(5, 4, 3), warm);
        mid = mixC(mid, gs::rgb4(9, 6, 3), warm);
        low = mixC(low, gs::rgb4(12, 8, 3), warm);
    }
    if (hot > 0.f) {
        top = mixC(top, gs::rgb4(4, 1, 2), hot);
        mid = mixC(mid, gs::rgb4(7, 2, 2), hot);
        low = mixC(low, gs::rgb4(8, 3, 2), hot);
    }
    constexpr float HORIZON = 92.f;
    const float span = float(gs::SCREEN_H) - HORIZON;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        if (y < int(HORIZON)) {
            v.road[y].on = false;
            float u = y / HORIZON;
            uint16_t c = u < 0.5f ? mixC(top, mid, u / 0.5f) : mixC(mid, low, (u - 0.5f) / 0.5f);
            v.lineBackdrop[y] = c;
            v.lineFog[y] = 0;
            continue;
        }
        float t = (float(y) + 0.5f - HORIZON) / span;
        t = std::max(t, 0.02f);
        float wz = 2.4f / t;
        gs::RoadLine& rd = v.road[y];
        rd.on = true;
        rd.cx = 168.f + shx * t * 0.4f;
        rd.hw = std::max(28.f, 5.4f * 78.f * t);
        rd.v = wz * 14.f;
        rd.pal = uint8_t(PAL_ROAD);
        rd.style = 1;
        rd.band = (int(std::floor(wz * 0.22f)) & 1) ? 1 : 0;
        rd.left = gs::GROUND_LAND;
        rd.right = gs::GROUND_LAND;
        float fogT = std::clamp((wz - 8.f) / 18.f, 0.f, 1.f);
        v.lineFog[y] = uint8_t(fogT * 10.f);
        v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
    }
    v.setFogColor(mode_ == Mode::Lost ? gs::rgb4(6, 2, 2) : gs::rgb4(2, 2, 4));
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();

    float shx = 0.f, shy = 0.f;
    if (shake_ > 0.f) {
        shx = std::sin(float(age_) * 1.7f) * shake_ * 5.f;
        shy = std::cos(float(age_) * 2.1f) * shake_ * 3.f;
    }
    sky(shx);

    const bool live = mode_ == Mode::Play || mode_ == Mode::Pause;
    const bool showPeople = live || mode_ == Mode::Title || mode_ == Mode::Lost;
    float lift = mode_ == Mode::Title ? 0.02f : lift_;
    float bot = doorBottom(lift) + shy;
    float cx = (DOOR_L + DOOR_R) * 0.5f + shx;
    int torch = int(sys_->frame / 8) & 1;
    int step = int(sys_->frame / 9) & 1;

    if (mode_ == Mode::Title) {
        text("S3 DEPOT DOOR", 160, 14, 0.92f, PAL_GOLD);
        text("HOLD THE DOOR", 160, 32, 0.55f, PAL_TEXT);
    } else if (mode_ == Mode::Won) {
        text("THE DOOR HELD", 160, 16, 0.62f, PAL_GOLD);
        text("THREE MINUTES", 160, 34, 0.48f, PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        text("THE DOOR OPENED", 160, 16, 0.52f, PAL_ALERT);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 16, 0.8f, PAL_GOLD);
    } else {
        int remain = std::max(0, HOLD - age_);
        int sec = (remain + 59) / 60;
        char clock[12];
        std::snprintf(clock, sizeof clock, "%d:%02d", sec / 60, sec % 60);
        text(clock, 160, 16, 1.15f, remain <= 600 ? PAL_ALERT : PAL_GOLD);
        if (dogLive_ && !dogAnswered_) text(dogSide_ == 0 ? "LEFT DOG" : "RIGHT DOG", 160, 36, 0.58f, PAL_ALERT);
        else if (truckLive_) text("TRUCK", 160, 36, 0.7f, PAL_ALERT);
        else if (missT_ > 0.f) text("LOOSE", missSide_ == 0 ? 78.f : 242.f, 48.f, 0.48f, PAL_ALERT);
    }

    for (const Spark& s : sparks_) {
        if (s.life <= 0.f) continue;
        spr(art_.spark, s.x + shx, s.y, 3.f + s.life * 8.f, PAL_LAMP);
    }

    bool fallen = mode_ == Mode::Lost;
    float clerkX = (fallen ? 196.f : (hauling_ ? 150.f : 158.f)) + shx;
    float clerkFoot = (fallen ? 206.f : 214.f) + shy;
    if (showPeople && !fallen) {
        float hx = clerkX + (hauling_ ? 2.f : 8.f);
        float hy = hauling_ ? 132.f : 148.f;
        float x0 = cx;
        float y0 = DOOR_TOP - 6.f + shy;
        int links = hauling_ ? 7 : 6;
        for (int i = 0; i < links; i++) {
            float t = (i + 0.5f) / float(links);
            float sag = std::sin(t * 3.1416f) * (hauling_ ? 2.f : 10.f);
            float x = x0 + (hx - x0) * t + sag;
            float y = y0 + (hy - y0) * t;
            spr(art_.chain, x, y, 7.f, PAL_IRON);
        }
    }
    if (showPeople) {
        if (fallen) spr(art_.clerk[2], clerkX, clerkFoot - 16.f, 32.f, PAL_COAT);
        else {
            const gs::Mipped& body = hauling_ ? art_.clerk[1] : art_.clerk[0];
            float h = hauling_ ? 86.f : 80.f;
            spr(body, clerkX, clerkFoot - h * 0.5f, h, PAL_COAT);
        }
        spr(art_.shadow, clerkX, clerkFoot - 2.f, 8.f, PAL_BAY, false, 0, true);
    }

    if (sack_ > 0.f || mode_ == Mode::Title) {
        float sy = (sack_ > 0.f ? bot + 2.f : DOOR_BOT - 2.f) + (mode_ == Mode::Title ? shy : 0.f);
        if (sack_ > 0.f || mode_ != Mode::Title) spr(art_.sack, cx - 10.f, sy, 16.f, PAL_FREIGHT);
    }

    auto boltAt = [&](float x, float slide, bool hot) {
        spr(art_.bolt, x + slide, DOOR_BOT + 2.f + shy, 8.f, hot ? PAL_ALERT : PAL_IRON);
    };
    if (mode_ != Mode::Lost) {
        boltAt(DOOR_L + 6.f + shx, -dogVisL_ * 10.f, dogVisL_ > 0.4f || (dogLive_ && dogSide_ == 0 && !dogAnswered_));
        boltAt(DOOR_R - 6.f + shx, dogVisR_ * 10.f, dogVisR_ > 0.4f || (dogLive_ && dogSide_ == 1 && !dogAnswered_));
    }

    int truckIn = live ? upcoming(TRUCK, nullptr) : -1;
    int truckTele = age_ > HOLD - 45 * 60 ? 50 : 72;
    float approach = 0.f;
    if (live && truckIn >= 0 && truckIn <= truckTele) approach = 1.f - float(truckIn) / float(truckTele);
    if (recoil_ > 0.f) approach = std::max(approach, std::clamp(recoil_ / 0.32f, 0.f, 1.f));
    if (mode_ == Mode::Title) approach = 0.15f;
    if (approach > 0.02f && mode_ != Mode::Won) {
        float tx = 318.f - approach * 78.f + shx;
        spr(art_.truck, tx, 168.f + shy, 36.f + approach * 6.f, PAL_FREIGHT, true);
        spr(art_.shadow, tx, 186.f, 8.f, PAL_BAY, false, 0, true);
    }

    bool showL = (dogLive_ && dogSide_ == 0) || dogVisL_ > 0.3f || mode_ == Mode::Title || mode_ == Mode::Lost;
    bool showR = (dogLive_ && dogSide_ == 1) || dogVisR_ > 0.3f || mode_ == Mode::Lost;
    if (showL && mode_ != Mode::Won) {
        float dx = (mode_ == Mode::Lost ? 118.f : 78.f) + shx;
        spr(art_.docker[step], dx, 176.f, mode_ == Mode::Lost ? 70.f : 62.f, PAL_DOCKER, false, mode_ == Mode::Lost ? 0 : 1);
    }
    if (showR && mode_ != Mode::Won) {
        spr(art_.docker[step ^ 1], 250.f + shx, 178.f, mode_ == Mode::Lost ? 66.f : 58.f, PAL_DOCKER, true, 1);
    }

    float pierH = 150.f;
    float pierY = 78.f + pierH * 0.5f + shy;
    spr(art_.pier, 58.f + shx, pierY, pierH, PAL_BRICK);
    spr(art_.pier, 262.f + shx, pierY, pierH, PAL_BRICK, true);
    stamp(art_.beam, cx, 56.f + shy, 210.f, 16.f, PAL_BRICK);
    stamp(art_.beam, 160.f + shx, 46.f + shy, 250.f, 10.f, PAL_IRON);
    spr(art_.sign, 160.f + shx, 40.f + shy, 18.f, PAL_GOLD);
    spr(art_.clock, 214.f + shx, 40.f, 16.f, PAL_GOLD);
    spr(art_.sprocket, cx, DOOR_TOP - 4.f + shy, 14.f, PAL_IRON);
    spr(art_.lamp[torch], 78.f + shx, 62.f, 18.f, PAL_LAMP);
    spr(art_.lamp[torch ^ 1], 242.f + shx, 62.f, 18.f, PAL_LAMP, true);

    if (mode_ != Mode::Lost) {
        float slatH = (DOOR_BOT - DOOR_TOP) / float(SLATS);
        for (int i = 0; i < SLATS; i++) {
            float y = bot - (i + 0.5f) * slatH;
            if (y + slatH * 0.5f < DOOR_TOP + shy) continue;
            stamp(art_.slat, cx, y, DOOR_W, slatH + 1.5f, PAL_STEEL, i & 1, (i & 1) ? 2 : 0);
        }
        stamp(art_.chip, DOOR_L + shx, (DOOR_TOP + DOOR_BOT) * 0.5f + shy, 4.f, DOOR_BOT - DOOR_TOP, PAL_IRON);
        stamp(art_.chip, DOOR_R + shx, (DOOR_TOP + DOOR_BOT) * 0.5f + shy, 4.f, DOOR_BOT - DOOR_TOP, PAL_IRON);
    }
    stamp(art_.bay, cx, (DOOR_TOP + DOOR_BOT) * 0.5f + shy, DOOR_W - 6.f, DOOR_BOT - DOOR_TOP, PAL_BAY);
    stamp(art_.chip, cx, DOOR_BOT + 5.f + shy, DOOR_W + 16.f, 4.f, PAL_GOLD);

    spr(art_.boxcar, 36.f, 128.f, 42.f, PAL_CAR, false, 7);
    spr(art_.crate, 286.f, 156.f, 28.f, PAL_FREIGHT, false, 3);
    spr(art_.crate, 304.f, 168.f, 22.f, PAL_FREIGHT, false, 4);
    spr(art_.drum, 24.f, 170.f, 26.f, PAL_FREIGHT, false, 2);
    spr(art_.drum, 292.f, 132.f, 20.f, PAL_FREIGHT, false, 5);

    spr(art_.moon, 28.f, 16.f, 14.f, PAL_NIGHT);
    static const int stars[][2] = {{64, 10}, {96, 18}, {128, 8}, {188, 12}, {236, 8}, {300, 14}, {276, 22}};
    for (int i = 0; i < 7; i++) {
        float tw = ((sys_->frame / 14 + i) % 5 == 0) ? 2.2f : 3.6f;
        spr(art_.star, float(stars[i][0]), float(stars[i][1]), tw, PAL_NIGHT);
    }

    if (live) {
        auto meter = [&](float x, float y, float w, float amt, int pal) {
            stamp(art_.chip, x + w * 0.5f, y, w, 5.f, PAL_BAY);
            float fw = w * std::clamp(amt, 0.f, 1.f);
            if (fw > 1.5f) stamp(art_.chip, x + fw * 0.5f, y, fw, 3.f, pal);
        };
        int armPal = arms_ < 0.25f ? PAL_ALERT : (hauling_ ? PAL_GOLD : PAL_GOOD);
        int openPal = lift_ > 0.62f ? PAL_ALERT : (lift_ > 0.35f ? PAL_GOLD : PAL_GOOD);
        meter(8.f, 12.f, 64.f, arms_, armPal);
        meter(248.f, 12.f, 64.f, lift_, openPal);
        hud(1, 0, "ARMS", PAL_TEXT);
        hud(33, 0, "OPEN", openPal == PAL_ALERT ? PAL_ALERT : PAL_TEXT);
        const char* state = "REST";
        int spal = PAL_TEXT;
        if (slip_ > 0.f) {
            state = "SLIP";
            spal = PAL_ALERT;
        } else if (dogL_ > 0.5f || dogR_ > 0.5f) {
            state = "DOG OUT";
            spal = PAL_ALERT;
        } else if (sack_ > 0.f) {
            state = "SACK";
            spal = PAL_GOOD;
        } else if (hauling_) {
            state = "HAUL";
            spal = PAL_GOLD;
        }
        hud(1, 26, state, spal);
        if (sackCd_ > 0.5f && sack_ <= 0.f) hud(30, 26, "SACK COOL", PAL_TEXT);
        else hud(31, 26, "SACK OK", PAL_GOOD);
        hudC(27, "ARROWS SET   X SACK   C HAULS", PAL_TEXT);
    } else if (mode_ == Mode::Title) {
        hudC(22, "YOU HAVE THE DEPOT", PAL_GOLD);
        hudC(23, "HOLD THE DOOR THREE MINUTES", PAL_TEXT);
        hudC(24, "ARROWS SET THE DOGS", PAL_TEXT);
        hudC(25, "X OR Z DROPS A SACK", PAL_TEXT);
        hudC(26, "C SPACE OR DOWN HAULS", PAL_TEXT);
        if ((sys_->frame / 30) & 1) hudC(27, "ENTER", PAL_GOLD);
    } else if (mode_ == Mode::Pause) {
        hudC(26, "ENTER RESUMES", PAL_TEXT);
        hudC(27, "ESC TITLE", PAL_TEXT);
    } else if (mode_ == Mode::Lost) {
        hudC(26, "THE DEPOT IS LOST", PAL_ALERT);
        hudC(27, "ENTER", PAL_TEXT);
    } else if (mode_ == Mode::Won) {
        hudC(26, "THE WATCH IS DONE", PAL_GOLD);
        hudC(27, "ENTER", PAL_TEXT);
    }

    if (mode_ == Mode::Won) sys_->setLight(40, 140, 64);
    else if (mode_ == Mode::Lost) sys_->setLight(170, 24, 16);
    else if (mode_ == Mode::Title) sys_->setLight(70, 46, 18);
    else if (truckLive_ || lift_ > 0.7f) sys_->setLight(170, 36, 16);
    else if (dogLive_ && !dogAnswered_) sys_->setLight(150, 48, 18);
    else if (hauling_) sys_->setLight(140, 96, 32);
    else sys_->setLight(90, 60, 24);
}

}  // namespace depotdoor

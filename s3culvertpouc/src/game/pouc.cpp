#include "game/pouc.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstring>

namespace culvertpouc {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float kFloor = 186.f;
constexpr float kRun = 132.f;
constexpr float kFree = 150.f;
constexpr float kDuckSp = 78.f;
constexpr float kAccel = 1400.f;
constexpr float kGrav = 820.f;
constexpr float kJumpV = -450.f;
constexpr float kWatch = 78.f;
constexpr float kSpawn = 84.f;
constexpr float kPouch0 = 196.f;
constexpr float kWin = 1508.f;
constexpr float kWorld = 1680.f;
constexpr float kSoffitA = 612.f;
constexpr float kSoffitB = 768.f;
constexpr float kGateX = 980.f;
constexpr float kGateHalf = 18.f;
constexpr float kGatePer = 4.4f;
constexpr float kGateBlock = 1.65f;
constexpr float kGatePhase = 0.35f;

struct Flow {
    float a, b;
};
constexpr Flow kFlows[2] = {{408.f, 500.f}, {1144.f, 1236.f}};

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

float wrap(float t, float per) {
    float u = std::fmod(t, per);
    if (u < 0.f) u += per;
    return u;
}

}  // namespace

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_ && won_) return 2;
    if (over_) return 3;
    return 1;
}

bool Game::overFlow(float x) const {
    for (const Flow& f : kFlows)
        if (x > f.a && x < f.b) return true;
    return false;
}

bool Game::gateShut(float t) const { return wrap(t + kGatePhase, kGatePer) < kGateBlock; }

float Game::gateOpen(float t) const {
    float u = wrap(t + kGatePhase, kGatePer);
    if (u < kGateBlock) return 0.f;
    return kGatePer - u;
}

bool Game::inSoffit(float x) const { return x > kSoffitA && x < kSoffitB; }

const gs::Mipped& Game::heroSprite() const {
    if (!onGround_) return art_.leap;
    if (duck_) return art_.duck;
    if (std::fabs(vx_) < 12.f) return art_.stand;
    return (int(stepT_) & 1) ? art_.runA : art_.runB;
}

void Game::blip(float freq, float vol, float hold) {
    beep_ = hold;
    sys_->apu.tone(0, freq, vol);
}

void Game::finish(bool crossed, const char* why) {
    if (mode_ != Mode::Play) return;
    won_ = crossed;
    over_ = true;
    cause_ = why;
    mode_ = crossed ? Mode::Won : Mode::Lost;
    fan_ = 0;
    fanT_ = 0.f;
    vx_ = 0.f;
    shake_ = crossed ? 0.f : 1.f;
    sys_->rumble(crossed ? 0.25f : 0.7f, crossed ? 0.4f : 0.2f, crossed ? 120 : 180);
}

void Game::dump(const char* where) const {
    std::fprintf(stderr, "culvertpouc %s x=%.1f pouch=%.1f held=%d cause=%s watch=%.1f\n", where, px_, pouchX_,
                 held_ ? 1 : 0, cause_, watchT_);
}

void Game::begin() {
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    held_ = false;
    onGround_ = true;
    duck_ = false;
    gateWas_ = gateShut(0.f);
    face_ = 1;
    fan_ = -1;
    lastSec_ = -1;
    cause_ = "";
    px_ = kSpawn;
    py_ = kFloor;
    vx_ = 0.f;
    vy_ = 0.f;
    pouchX_ = kPouch0;
    watchT_ = 0.f;
    stepT_ = 0.f;
    jumpBuf_ = 0.f;
    shake_ = 0.f;
    cam_ = 0.f;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& duck) {
    float goal = held_ ? kWin + 12.f : pouchX_;
    bool goR = goal > px_ + 2.f;
    bool goL = goal < px_ - 2.f;

    if (inSoffit(px_) || (goR && px_ > kSoffitA - 18.f && px_ < kSoffitB)) duck = true;

    if (!onGround_) {
        if (goR) right = true;
        if (goL) left = true;
        return;
    }

    if (goR) {
        for (const Flow& f : kFlows) {
            if (px_ >= f.a - 36.f && px_ < f.a - 2.f) {
                if (vx_ < 100.f && px_ > f.a - 16.f) left = true;
                else {
                    right = true;
                    if (vx_ > 118.f) jump = true;
                }
                return;
            }
        }
    }

    if (goR && px_ > kGateX - 120.f && px_ < kGateX + kGateHalf + 16.f) {
        float near = kGateX - kGateHalf - 36.f;
        float far = kGateX + kGateHalf + 14.f;
        bool shut = gateShut(watchT_);
        float op = gateOpen(watchT_);
        float need = (far - std::max(px_, near)) / kRun + 0.35f;
        if (px_ < near - 2.f) {
            right = true;
        } else if (px_ < kGateX - kGateHalf - 4.f) {
            if (!shut && op > std::max(1.15f, need)) right = true;
            else if (px_ > near + 4.f) left = true;
        } else {
            right = true;
        }
        return;
    }

    if (goR) {
        right = true;
        face_ = 1;
    } else if (goL) {
        left = true;
        face_ = -1;
    }
}

void Game::stepPlay(bool left, bool right, bool jump, bool duck) {
    watchT_ += DT;
    int sec = int(watchT_);
    if (sec != lastSec_ && sec > 0 && mode_ == Mode::Play) {
        lastSec_ = sec;
        blip(sec >= int(kWatch) - 10 ? 880.f : 160.f, 0.02f, 0.03f);
    }
    bool shut = gateShut(watchT_);
    if (shut && !gateWas_) sys_->apu.noiseBurst(0.18f, 180.f, 0.09f);
    gateWas_ = shut;

    duck_ = duck && onGround_;
    float spd = duck_ ? kDuckSp : (held_ ? kRun : kFree);
    float target = 0.f;
    if (right && !left) target = spd;
    if (left && !right) target = -spd;
    if (right != left) face_ = right ? 1 : -1;
    vx_ = approach(vx_, target, kAccel * DT);

    if (jump) jumpBuf_ = 0.12f;
    else jumpBuf_ = std::max(0.f, jumpBuf_ - DT);
    if (jumpBuf_ > 0.f && onGround_ && !duck_) {
        vy_ = kJumpV;
        onGround_ = false;
        jumpBuf_ = 0.f;
        blip(380.f, 0.03f, 0.03f);
    }
    if (!onGround_) vy_ = std::min(540.f, vy_ + kGrav * DT);
    else vy_ = 0.f;

    px_ += vx_ * DT;
    py_ += vy_ * DT;
    px_ = std::clamp(px_, 28.f, kWorld - 36.f);

    if (overFlow(px_)) {
        onGround_ = false;
        if (py_ >= kFloor + 10.f && vy_ >= 0.f) {
            finish(false, "FLOW");
            return;
        }
    } else if (py_ >= kFloor) {
        py_ = kFloor;
        vy_ = 0.f;
        onGround_ = true;
    } else {
        onGround_ = false;
    }
    if (mode_ != Mode::Play) return;

    if (onGround_ && std::fabs(vx_) > 20.f) stepT_ += std::fabs(vx_) * DT * 0.18f;

    if (inSoffit(px_) && !duck_ && onGround_) {
        px_ = kSoffitA - 8.f;
        vx_ = -70.f;
        py_ = kFloor;
        blip(120.f, 0.05f, 0.06f);
        shake_ = 0.6f;
    }

    if (shut && std::fabs(px_ - kGateX) < kGateHalf + 6.f) {
        finish(false, "GATE");
        return;
    }

    if (held_) {
        pouchX_ = px_ + float(face_) * 11.f;
    } else if (onGround_ && std::fabs(px_ - pouchX_) < 20.f && (kFloor - py_) < 8.f) {
        held_ = true;
        blip(640.f, 0.07f, 0.08f);
        sys_->rumble(0.2f, 0.4f, 60);
        sys_->setLight(190, 140, 50);
    } else {
        if (shut && std::fabs(pouchX_ - kGateX) < kGateHalf) {
            finish(false, "GATE");
            return;
        }
        if (overFlow(pouchX_)) {
            finish(false, "FLOW");
            return;
        }
    }

    if (onGround_ && px_ >= kWin) {
        if (!held_) {
            finish(false, "EMPTY");
            return;
        }
        finish(true, "CROSSED");
        return;
    }
    if (mode_ == Mode::Play && watchT_ >= kWatch) finish(false, "TIME");
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

void Game::stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip, int fog, bool shadow) {
    if (w < 1.f || h < 1.f || m.h < 1) return;
    gs::Sprite s;
    s.w = int16_t(std::lround(std::clamp(w, 1.f, 2000.f)));
    s.h = int16_t(std::lround(std::clamp(h, 1.f, 2000.f)));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 80 || s.x + s.w < -80 || s.y > gs::SCREEN_H + 80 || s.y + s.h < -80) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.fog = uint8_t(std::clamp(fog, 0, 16));
    s.hflip = flip;
    s.shadow = shadow;
    sys_->vdp.sprite(s);
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet, int fog, bool shadow) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    stamp(m, cx, feet ? cy - h * 0.5f : cy, w, h, pal, flip, fog, shadow);
}

void Game::text(const char* s, float x, float y, float scale, int pal, int align) {
    if (!s) return;
    float width = 0.f;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (const char* p = s; *p; p++) {
        unsigned char c = static_cast<unsigned char>(*p);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, g.h * scale, pal, false, false, 0, false);
        x += g.w * scale + scale;
    }
}

void Game::sky(float cam) {
    gs::VDP& vdp = sys_->vdp;
    for (int y = 0; y < gs::SCREEN_H; y++) {
        uint16_t c;
        if (y < 36) c = gs::rgb4(3, 5, 8);
        else if (y < 58) c = gs::rgb4(2, 2, 3);
        else if (y < 168) c = gs::rgb4(2, 3, 4);
        else c = gs::rgb4(1, 3, 3);
        vdp.lineBackdrop[y] = c;
        vdp.lineFog[y] = uint8_t(y > 150 ? (y - 150) / 10 : 0);
        vdp.A.hscroll[y] = int16_t(cam * 0.35f);
        vdp.B.hscroll[y] = int16_t(cam * 0.15f);
        vdp.A.vscroll[y] = 0;
        vdp.B.vscroll[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = true;
    vdp.B.enabled = true;
    vdp.hudEnabled = true;
}

void Game::drawWorld(float cam) {
    auto feet = [&](const gs::Mipped& m, float wx, float foot, float h, int pal, bool flip = false, int fog = 0) {
        spr(m, wx - cam, foot, h, pal, flip, true, fog, false);
    };
    int fi = int(t_ * 8.f) & 1;
    int bob = int(t_ * 5.f) & 1;
    float hy = std::min(py_, kFloor + 36.f);
    if (held_) {
        float bobY = std::sin(stepT_ * 0.7f) * 2.f;
        spr(art_.pouch[bob], px_ + float(face_) * 12.f - cam, hy - (duck_ ? 16.f : 28.f) + bobY, 18.f, PAL_POUCH,
            face_ < 0, false, 0, false);
    } else {
        feet(art_.pouch[bob], pouchX_, kFloor - 2.f, 20.f, PAL_POUCH, false, 0);
    }
    float lift = std::max(0.f, kFloor - py_);
    spr(art_.shadow, px_ - cam, kFloor + 2.f, std::max(4.f, 8.f - lift * 0.04f), PAL_STONE, false, true, 0, true);
    feet(heroSprite(), px_, hy, duck_ ? 36.f : 52.f, PAL_PLAYER, face_ < 0, 0);

    bool shut = gateShut(watchT_);
    float drop = shut ? (kFloor - 4.f) : (kFloor - 78.f);
    feet(art_.gate, kGateX, drop, shut ? 78.f : 36.f, PAL_GATE, false, 0);

    for (const Flow& f : kFlows) {
        float mid = (f.a + f.b) * 0.5f;
        float w = (f.b - f.a) + 8.f;
        float wave = std::sin(t_ * 3.f + mid) * 2.f;
        feet(art_.lip, f.a, kFloor + 2.f, 18.f, PAL_STONE, false, 0);
        feet(art_.lip, f.b, kFloor + 2.f, 18.f, PAL_STONE, true, 0);
        stamp(art_.water, mid - cam, kFloor + 18.f + wave, w, 22.f, PAL_FLOW, false, 0, false);
        feet(art_.weed, f.a + 14.f, kFloor + 4.f, 16.f, PAL_FLOW, false, 0);
        feet(art_.weed, f.b - 14.f, kFloor + 6.f, 14.f, PAL_FLOW, true, 0);
    }

    stamp(art_.soffit, (kSoffitA + kSoffitB) * 0.5f - cam, kFloor - 58.f, kSoffitB - kSoffitA, 16.f, PAL_STONE, false,
          0, false);

    const float rings[] = {140.f, 300.f, 560.f, 860.f, 1080.f, 1360.f, 1560.f};
    for (float x : rings) feet(art_.ring, x, kFloor + 6.f, 108.f, PAL_STONE, false, 4);

    const float lamps[] = {250.f, 700.f, 900.f, 1320.f};
    for (float x : lamps) {
        if (overFlow(x)) continue;
        feet(art_.lamp, x, kFloor, 48.f, PAL_STONE, false, 0);
        spr(art_.flame[fi], x - cam, kFloor - 50.f, 8.f, PAL_LAMP, false, false, 0, false);
    }

    for (float x = 40.f; x < kWorld; x += 70.f)
        stamp(art_.deck, x - cam * 0.55f, 52.f, 78.f, 14.f, PAL_ROAD, false, 6, false);
    float truck = std::fmod(t_ * 28.f, kWorld + 80.f) - 40.f;
    spr(art_.truck, truck - cam * 0.55f, 40.f, 14.f, PAL_ROAD, false, false, 5, false);

    feet(art_.mark, kWin - 8.f, kFloor, 40.f, PAL_LAMP, false, 0);
    spr(art_.flame[fi], kWin - 8.f - cam, kFloor - 42.f, 10.f, PAL_LAMP, false, false, 0, false);
}

void Game::drawTitle() {
    int bob = int(t_ * 5.f) & 1;
    int fi = int(t_ * 8.f) & 1;
    text("S3 CULVERT POUC", 160.f, 18.f, 1.05f, PAL_HUD, 0);
    text("CARRY THE POUCH ACROSS", 160.f, 40.f, 0.72f, PAL_LAMP, 0);
    text("MISS IT AND THE WATCH IS OVER", 160.f, 56.f, 0.55f, PAL_ALERT, 0);
    if ((int(t_ * 2.f) & 1) == 0) text("START", 160.f, 76.f, 0.9f, PAL_HUD, 0);

    spr(art_.truck, 40.f + std::fmod(t_ * 18.f, 240.f), 36.f, 16.f, PAL_ROAD, false, false, 0, false);
    stamp(art_.deck, 160.f, 52.f, 300.f, 16.f, PAL_ROAD, false, 0, false);
    spr(art_.ring, 70.f, kFloor + 4.f, 100.f, PAL_STONE, false, true, 2, false);
    spr(art_.ring, 250.f, kFloor + 4.f, 100.f, PAL_STONE, false, true, 2, false);
    spr(art_.soffit, 160.f, kFloor - 52.f, 120.f, PAL_STONE, false, false, 0, false);
    spr(art_.water, 160.f, kFloor + 16.f, 28.f, PAL_FLOW, false, false, 0, false);
    spr(art_.pouch[bob], 118.f, kFloor - 8.f, 22.f, PAL_POUCH, false, true, 0, false);
    spr(art_.stand, 88.f, kFloor, 52.f, PAL_PLAYER, false, true, 0, false);
    spr(art_.shadow, 88.f, kFloor + 2.f, 8.f, PAL_STONE, false, true, 0, true);
    spr(art_.gate, 210.f, kFloor - 10.f, 70.f, PAL_GATE, false, true, 0, false);
    spr(art_.lamp, 250.f, kFloor, 46.f, PAL_STONE, false, true, 0, false);
    spr(art_.flame[fi], 250.f, kFloor - 48.f, 9.f, PAL_LAMP, false, false, 0, false);
    hudC(26, "ARROWS MOVE   Z JUMP   DOWN DUCK", PAL_HUD);
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();
    float cam = 0.f;
    if (mode_ != Mode::Title) {
        cam = cam_;
        if (shake_ > 0.f) cam += std::sin(t_ * 60.f) * shake_ * 3.f;
        cam = std::clamp(cam, 0.f, kWorld - float(gs::SCREEN_W));
    }
    sky(cam);
    if (mode_ == Mode::Title) {
        drawTitle();
        return;
    }
    if (mode_ == Mode::Won) {
        text("THE POUCH CROSSED", 160.f, 20.f, 1.0f, PAL_HUD, 0);
        text("THE WATCH HOLDS", 160.f, 42.f, 0.8f, PAL_LAMP, 0);
    } else if (mode_ == Mode::Lost) {
        text("THE WATCH IS OVER", 160.f, 20.f, 0.95f, PAL_ALERT, 0);
        const char* why = "THE CLOCK DIED";
        if (cause_[0] == 'F') why = "THE FLOW TOOK IT";
        else if (cause_[0] == 'G') why = "THE GATE CAME DOWN";
        else if (cause_[0] == 'E') why = "CROSSED WITH EMPTY HANDS";
        text(why, 160.f, 42.f, 0.65f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160.f, 24.f, 1.1f, PAL_HUD, 0);
    }
    drawWorld(cam);

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = int(std::ceil(kWatch - watchT_));
        if (left < 0) left = 0;
        char buf[24];
        std::snprintf(buf, sizeof buf, "WATCH %d", left);
        hud(1, 0, buf, left <= 10 ? PAL_ALERT : PAL_HUD);
        hud(31, 0, held_ ? "POUCH" : "EMPTY", held_ ? PAL_LAMP : PAL_ALERT);
        const char* hint = held_ ? "CARRY IT THROUGH" : "TAKE THE POUCH";
        int hintPal = PAL_HUD;
        if (inSoffit(px_)) {
            hint = "DUCK THE SOFFIT";
            hintPal = duck_ ? PAL_LAMP : PAL_ALERT;
        } else if (px_ > kGateX - 90.f && px_ < kGateX + 40.f) {
            bool shut = gateShut(watchT_);
            hint = shut ? "WAIT FOR THE GATE" : "THROUGH THE GATE";
            hintPal = shut ? PAL_ALERT : PAL_LAMP;
        } else {
            for (const Flow& f : kFlows) {
                if (px_ > f.a - 80.f && px_ < f.a) {
                    hint = "JUMP THE FLOW";
                    hintPal = PAL_FLOW;
                }
            }
        }
        hudC(1, hint, hintPal);
        hud(1, 26, "ARROWS  Z JUMP  DOWN DUCK", PAL_HUD);
    } else if (!bot_) {
        hudC(26, "START", PAL_HUD);
    }
}

void Game::serviceAudio() {
    if (beep_ > 0.f) {
        beep_ -= DT;
        if (beep_ <= 0.f) sys_->apu.tone(0, 0, 0);
    }
    if (fan_ >= 0) {
        static const float good[] = {294.f, 370.f, 440.f, 587.f};
        static const float bad[] = {196.f, 164.f, 130.f, 98.f};
        fanT_ += DT;
        if (fanT_ > 0.16f) {
            const float* notes = won_ ? good : bad;
            if (fan_ < 4) sys_->apu.tone(2, notes[fan_], won_ ? 0.07f : 0.04f);
            else sys_->apu.tone(2, 0, 0);
            fan_++;
            fanT_ = 0.f;
            if (fan_ > 8) fan_ = -1;
        }
        sys_->apu.tone(1, 0, 0);
        return;
    }
    if (mode_ == Mode::Play) sys_->apu.tone(1, held_ ? 82.f : 55.f, 0.014f);
    else if (mode_ == Mode::Title) sys_->apu.tone(1, 46.f, 0.012f);
    else sys_->apu.tone(1, 0, 0);
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.apu.setMaster(0.66f);
    t_ = 0.f;
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    cam_ = 0.f;
    if (bot_) begin();
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    const gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
        else if (!bot_ && pad.pressed(gs::BTN_MODE)) {
            if (sys.hasHome()) sys.eject();
            else sys.quit();
        }
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Play;
            blip(440.f, 0.04f, 0.04f);
        } else if (pad.pressed(gs::BTN_MODE)) {
            mode_ = Mode::Title;
            over_ = false;
            won_ = false;
            sys.apu.tone(1, 0, 0);
        }
    } else if (mode_ == Mode::Won || mode_ == Mode::Lost) {
        if (!bot_ && (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C))) begin();
    } else if (mode_ == Mode::Play) {
        bool left = false, right = false, jump = false, duck = false;
        if (bot_) {
            bot(left, right, jump, duck);
        } else if (pad.pressed(gs::BTN_START)) {
            mode_ = Mode::Pause;
            blip(260.f, 0.04f, 0.04f);
        } else {
            left = pad.down(gs::BTN_LEFT) || pad.axisX <= -0.35f;
            right = pad.down(gs::BTN_RIGHT) || pad.axisX >= 0.35f;
            jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_UP) ||
                   pad.pressed(gs::BTN_TURBO);
            duck = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump, duck);
        float want = std::clamp(px_ - 130.f, 0.f, kWorld - float(gs::SCREEN_W));
        cam_ += (want - cam_) * std::min(1.f, DT * 8.f);
    }

    if (mode_ != Mode::Pause && shake_ > 0.f) shake_ = std::max(0.f, shake_ - DT * 2.2f);

    if (mode_ == Mode::Won) sys.setLight(40, 150, 80);
    else if (mode_ == Mode::Lost) sys.setLight(150, 30, 20);
    else if (mode_ == Mode::Title) sys.setLight(90, 80, 40);
    else if (held_) sys.setLight(160, 110, 40);
    else sys.setLight(50, 70, 110);

    serviceAudio();
    draw();
}

}  // namespace culvertpouc

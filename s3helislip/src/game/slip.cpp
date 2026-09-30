#include "game/slip.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace slip {
namespace {

constexpr float DT = 1.f / 60.f;
constexpr float GRAV = 92.f;
constexpr float HY = 16.f;
constexpr float SLIP_L = 198.f;
constexpr float SLIP_R = 242.f;
constexpr float SLIP_X = 220.f;
constexpr float DECK = 168.f;
constexpr float PILE_TOP = 108.f;
constexpr float PILE_L0 = 156.f;
constexpr float PILE_L1 = 184.f;
constexpr float PILE_R0 = 256.f;
constexpr float PILE_R1 = 284.f;
constexpr float WATER0 = 204.f;
constexpr float WATER1 = 146.f;
constexpr float CREW_TIME = 20.f;

bool hitPile(float x, float y) {
    float top = y - 8.f;
    float bot = y + HY;
    float left = x - 12.f;
    float right = x + 12.f;
    if (bot < PILE_TOP || top > 224.f) return false;
    bool L = right > PILE_L0 && left < PILE_L1;
    bool R = right > PILE_R0 && left < PILE_R1;
    return L || R;
}

}  // namespace

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(long(std::lround(w)), 1L, 2000L));
    s.h = int16_t(std::clamp(long(std::lround(h)), 1L, 2000L));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.x + s.w < -48 || s.y > gs::SCREEN_H + 40 || s.y + s.h < -40) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal) {
    for (char ch : s) {
        if (ch < 32 || ch > 126) {
            x += 6.f * scale;
            continue;
        }
        const gs::Mipped& m = art_.glyph[ch - 32];
        float h = std::max(7.f * scale, 1.f);
        spr(m, x + h * 0.4f, y, h, pal);
        x += (ch == ' ' ? 5.f : 6.2f) * scale;
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.setFogColor(gs::rgb4(6, 9, 12));
    resetFlight();
    mode_ = Mode::Title;
}

void Game::resetFlight() {
    x_ = 48.f;
    y_ = 58.f;
    vx_ = vy_ = 0;
    hold_ = 0;
    t_ = 0;
    over_ = false;
    won_ = false;
    loseKind_ = 0;
    waterY_ = WATER0;
    rivalX_ = -40.f;
    rivalY_ = 40.f;
}

void Game::update(float dt) {
    const gs::Pad& pad = sys_->pad;
    bool up = pad.down(gs::BTN_UP) || pad.down(gs::BTN_A) || pad.down(gs::BTN_C);
    bool down = pad.down(gs::BTN_DOWN) || pad.down(gs::BTN_B);
    bool left = pad.down(gs::BTN_LEFT);
    bool right = pad.down(gs::BTN_RIGHT);
    float stickX = pad.axisX;
    if (left) stickX = -1;
    if (right) stickX = 1;

    float lift = GRAV * 0.7f;
    float thrust = stickX * 92.f;
    if (up) lift = GRAV * 1.75f;
    if (down) lift = 0;

    float tideU = std::clamp(t_ / CREW_TIME, 0.f, 1.f);
    waterY_ = WATER0 + (WATER1 - WATER0) * tideU;

    if (bot_) {
        float tx = SLIP_X;
        float ty = 54.f;
        bool lined = std::fabs(x_ - SLIP_X) < 5.f && std::fabs(vx_) < 14.f;
        if (lined) ty = DECK - HY + 8.f;
        else if (y_ > 72.f) ty = 48.f;
        float ux = std::clamp(2.6f * (tx - x_) - 3.6f * vx_, -110.f, 110.f);
        float uy = std::clamp(3.0f * (ty - y_) - 4.0f * vy_, -140.f, 160.f);
        thrust = ux;
        lift = std::clamp(GRAV - uy, 0.f, GRAV * 2.4f);
        if (lined && y_ > DECK - HY - 18.f) thrust *= 0.35f;
    }

    vx_ += thrust * dt;
    vy_ += (GRAV - lift) * dt;
    vx_ *= std::exp(-1.35f * dt);
    x_ += vx_ * dt;
    y_ += vy_ * dt;

    if (x_ < 20.f) {
        x_ = 20.f;
        vx_ = std::max(0.f, vx_);
    }
    if (x_ > 300.f) {
        x_ = 300.f;
        vx_ = std::min(0.f, vx_);
    }
    if (y_ < 14.f) {
        y_ = 14.f;
        vy_ = std::max(0.f, vy_);
    }

    float crewU = tideU;
    rivalX_ = -36.f + (SLIP_X - 8.f - -36.f) * std::min(crewU * 1.15f, 1.f);
    if (crewU < 0.78f) rivalY_ = 42.f + std::sin(t_ * 1.6f) * 3.f;
    else rivalY_ = 42.f + (DECK - HY - 42.f) * ((crewU - 0.78f) / 0.22f);

    if (hitPile(x_, y_)) {
        loseKind_ = 2;
        mode_ = Mode::Lose;
        msgT_ = 1.6f;
        over_ = bot_;
        sys_->apu.tone(1, 90, 0.18f);
        return;
    }

    float feet = y_ + HY;
    bool inSlip = x_ > SLIP_L + 2.f && x_ < SLIP_R - 2.f;
    bool onDeck = false;
    if (inSlip && feet >= DECK) {
        if (waterY_ <= DECK + 1.f) {
            loseKind_ = 3;
            mode_ = Mode::Lose;
            msgT_ = 1.8f;
            over_ = bot_;
            sys_->apu.tone(1, 100, 0.2f);
            return;
        }
        y_ = DECK - HY;
        if (vy_ > 96.f) {
            vy_ = -vy_ * 0.2f;
            sys_->apu.tone(1, 150, 0.07f);
        } else if (vy_ > 0) {
            vy_ = 0;
        }
        vx_ *= 0.72f;
        onDeck = std::fabs(vy_) < 8.f;
    } else if (!inSlip && feet >= waterY_) {
        loseKind_ = 1;
        mode_ = Mode::Lose;
        msgT_ = 1.6f;
        over_ = bot_;
        sys_->apu.tone(1, 70, 0.22f);
        return;
    }

    float speed = std::sqrt(vx_ * vx_ + vy_ * vy_);
    if (onDeck && speed < 10.f && waterY_ > DECK + 4.f) hold_ += dt;
    else hold_ = 0;

    if (hold_ > 0.35f) {
        won_ = true;
        over_ = true;
        mode_ = Mode::Win;
        msgT_ = 2.2f;
        sys_->apu.tone(1, 680, 0.16f);
        return;
    }

    if (t_ >= CREW_TIME || waterY_ <= DECK + 1.f) {
        loseKind_ = 3;
        mode_ = Mode::Lose;
        msgT_ = 1.8f;
        over_ = bot_;
        sys_->apu.tone(1, 100, 0.2f);
        return;
    }

    sys_->apu.tone(0, 40.f + std::fabs(lift - GRAV) * 0.07f, 0.045f);
    rotor_ = int(t_ * (18.f + std::fabs(lift - GRAV) * 0.04f)) % 3;
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    int wy = int(std::lround(waterY_));
    for (int y = 0; y < gs::SCREEN_H; y++) {
        float u = y / float(gs::SCREEN_H - 1);
        int r, g, b;
        if (y >= wy) {
            float d = (y - wy) / 40.f;
            r = 1;
            g = int(std::max(2.f, 7.f - d * 3.f));
            b = int(std::max(5.f, 12.f - d * 3.f));
        } else {
            r = int(5 + 4 * u);
            g = int(8 + 3 * u);
            b = int(12 + 2 * u);
        }
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
        vdp.road[y].on = false;
    }
    vdp.A.enabled = false;
    vdp.B.enabled = false;
    vdp.HUD.clear();

    spr(art_.cloud, 70 + std::sin(t_ * 0.18f) * 10.f, 28, 18, PAL_SEA);
    spr(art_.cloud, 230, 42, 14, PAL_SEA);
    spr(art_.gull, 120 + std::sin(t_ * 0.9f) * 40.f, 52 + std::sin(t_ * 1.7f) * 4.f, 10, PAL_SEA);
    spr(art_.gull, 40 + t_ * 6.f, 70, 8, PAL_SEA, true);

    spr(art_.gauge, 16, 150, 64, PAL_PIER);
    float mark = 118.f + (waterY_ - WATER1) / (WATER0 - WATER1) * 48.f;
    spr(art_.buoy, 92, waterY_ - 8.f, 28, PAL_SEA);

    for (int i = 0; i < 5; i++) {
        float wx = std::fmod(i * 70.f - t_ * 14.f, 380.f);
        if (wx < 0) wx += 380.f;
        spr(art_.wave, wx - 20.f, waterY_ + 6.f, 16, PAL_SEA);
    }

    spr(art_.pile, (PILE_L0 + PILE_L1) * 0.5f, 166, 112, PAL_PIER);
    spr(art_.pile, (PILE_R0 + PILE_R1) * 0.5f, 166, 112, PAL_PIER);
    spr(art_.deck, SLIP_X, DECK + 2.f, 16, PAL_PIER);

    bool faceL = vx_ < -6.f;
    spr(art_.body, x_, y_, 38, PAL_SHIP, faceL);
    spr(art_.rotor[rotor_], x_ + (faceL ? -6.f : 6.f), y_ - 18.f, 14, PAL_SHIP, faceL);

    int rframe = int(t_ * 16.f) % 3;
    spr(art_.rival, rivalX_, rivalY_, 26, PAL_RIVAL, false);
    spr(art_.rotor[rframe], rivalX_ + 4.f, rivalY_ - 12.f, 9, PAL_RIVAL);

    (void)mark;
    if (mode_ == Mode::Title) {
        text("S3 HELISLIP", 78, 64, 2.0f, PAL_HUD);
        text("BERTH IN THE SLIP", 68, 90, 1.35f, PAL_HUD);
        text("BEFORE THE TIDE TURNS", 52, 108, 1.2f, PAL_HUD);
        text("ARROWS FLY   ENTER START", 48, 130, 1.1f, PAL_HUD);
    } else if (mode_ == Mode::Win) {
        text("BERTHED", 108, 36, 2.1f, PAL_HUD);
        text("IN THE SLIP", 96, 58, 1.4f, PAL_HUD);
    } else if (mode_ == Mode::Lose) {
        const char* msg = "TIDE TURNED";
        if (loseKind_ == 1) msg = "IN THE DRINK";
        else if (loseKind_ == 2) msg = "SCRAPED A PILE";
        text(msg, 70, 40, 1.6f, PAL_HUD);
    } else {
        int left = std::max(0, int(std::ceil(CREW_TIME - t_)));
        char buf[40];
        std::snprintf(buf, sizeof(buf), "CREW %d", left);
        text(buf, 8, 12, 1.3f, PAL_HUD);
        float speed = std::sqrt(vx_ * vx_ + vy_ * vy_);
        std::snprintf(buf, sizeof(buf), "SPD %d", int(std::lround(speed)));
        text(buf, 196, 12, 1.2f, PAL_HUD);
        bool lit = x_ > SLIP_L && x_ < SLIP_R && (y_ + HY) >= DECK - 1.f;
        text(lit ? "SLIP" : "OPEN", 250, 28, 1.15f, PAL_HUD);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    const gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
    if (bot_ && mode_ == Mode::Title) start = true;

    if (mode_ == Mode::Title) {
        t_ += DT;
        if (start) {
            resetFlight();
            mode_ = Mode::Fly;
        }
    } else if (mode_ == Mode::Fly) {
        t_ += DT;
        update(DT);
    } else if (mode_ == Mode::Lose) {
        msgT_ -= DT;
        sys.apu.tone(0, 0, 0);
        if (msgT_ <= 0 || start) {
            if (bot_) over_ = true;
            else {
                resetFlight();
                mode_ = Mode::Fly;
            }
        }
    } else if (mode_ == Mode::Win) {
        msgT_ -= DT;
        t_ += DT;
        sys.apu.tone(0, 0, 0);
        if (msgT_ < 1.3f) sys.apu.tone(1, 0, 0);
        if (msgT_ <= 0) over_ = true;
    }
    if (mode_ != Mode::Fly) rotor_ = int(t_ * 12.f) % 3;
    draw();
}

}  // namespace slip

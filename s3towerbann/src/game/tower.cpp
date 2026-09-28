#include "game/tower.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <string>

namespace towerbann {
namespace {

constexpr float DT = 1.0f / 60.0f;
constexpr int NF = 10;
constexpr float kStep = 108.0f;
constexpr float kL = 36.0f;
constexpr float kR = 284.0f;
constexpr float kRun = 150.0f;
constexpr float kAccel = 1400.0f;
constexpr float kJump = 548.0f;
constexpr float kGrav = 1040.0f;
constexpr float kDoor = 160.0f;
constexpr int kCrowFloor = 5;

float approach(float v, float target, float delta) {
    if (v < target) return std::min(target, v + delta);
    return std::max(target, v - delta);
}

float floorY(int i) { return float(i) * kStep; }

void gapOf(int i, float& g0, float& g1) {
    if (i <= 0) {
        g0 = g1 = 0;
        return;
    }
    if (i & 1) {
        g0 = 150.0f;
        g1 = 230.0f;
    } else {
        g0 = 70.0f;
        g1 = 150.0f;
    }
}

bool solidAt(int i, float x) {
    if (x < kL || x > kR) return false;
    float g0, g1;
    gapOf(i, g0, g1);
    if (g1 <= g0) return true;
    if (x >= g0 && x <= g1) return false;
    return true;
}

float landX(int i) {
    if (i <= 0) return kDoor;
    float g0, g1;
    gapOf(i, g0, g1);
    float leftW = g0 - kL;
    float rightW = kR - g1;
    if (leftW >= rightW) return g0 - 22.0f;
    return g1 + 22.0f;
}

float gapCenter(int i) {
    float g0, g1;
    gapOf(i, g0, g1);
    if (g1 <= g0) return landX(i);
    return (g0 + g1) * 0.5f;
}

}  // namespace

const gs::Mipped& Game::hero() const {
    if (!grounded_) return art_.jump;
    if (std::abs(vx_) > 16.0f) return (int(playT_ * 8.0f) & 1) ? art_.walkA : art_.walkB;
    return art_.stand;
}

int Game::marker() const {
    if (mode_ == Mode::Title) return 0;
    if (over_) return 4;
    if (!has_) return 1;
    if (floor_ >= 6) return 2;
    return 3;
}

void Game::resetRun() {
    px_ = kDoor;
    py_ = 0;
    vx_ = vy_ = 0;
    face_ = 1;
    grounded_ = true;
    floor_ = 0;
    has_ = false;
    bannerFloor_ = NF - 1;
    bannerX_ = landX(NF - 1);
    lives_ = 4;
    crowX_ = 200.0f;
    crowDir_ = -1.0f;
    airTargetX_ = px_;
    playT_ = inv_ = stun_ = shake_ = 0;
}

void Game::begin() {
    resetRun();
    mode_ = Mode::Play;
    over_ = false;
    won_ = false;
    blip(440.0f, 0.05f, 0.08f);
}

void Game::blip(float freq, float vol, float hold) {
    sys_->apu.tone(0, freq, vol);
    beep_ = hold;
}

void Game::win() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Victory;
    won_ = true;
    over_ = true;
    vx_ = vy_ = 0;
    shake_ = 0.3f;
    sys_->rumble(0.25f, 0.55f, 180);
    sys_->setLight(40, 120, 70);
    blip(660.0f, 0.08f, 0.22f);
}

void Game::lose() {
    if (mode_ != Mode::Play) return;
    mode_ = Mode::Over;
    won_ = false;
    over_ = true;
    vx_ = vy_ = 0;
    sys_->rumble(0.7f, 0.2f, 160);
    sys_->setLight(120, 20, 24);
    sys_->apu.noiseBurst(0.35f, 120.0f, 0.25f);
}

void Game::peck() {
    if (inv_ > 0 || stun_ > 0 || mode_ != Mode::Play) return;
    lives_--;
    inv_ = 1.4f;
    stun_ = 0.28f;
    vy_ = 180.0f;
    grounded_ = false;
    vx_ = crowX_ >= px_ ? -120.0f : 120.0f;
    shake_ = 0.7f;
    sys_->apu.noiseBurst(0.4f, 420.0f, 0.12f);
    sys_->rumble(0.5f, 0.25f, 90);
    if (has_) {
        has_ = false;
        int f = floor_;
        if (!grounded_) {
            f = 0;
            for (int i = 0; i < NF; i++)
                if (floorY(i) <= py_) f = i;
        }
        if (!solidAt(f, px_)) bannerX_ = landX(f);
        else bannerX_ = std::clamp(px_, kL + 8.0f, kR - 8.0f);
        bannerFloor_ = f;
    }
    if (lives_ <= 0) lose();
}

void Game::bot(bool& left, bool& right, bool& jump) {
    left = right = jump = false;
    auto go = [&](float tx) {
        if (px_ < tx - 3.0f) {
            right = true;
            face_ = 1;
        } else if (px_ > tx + 3.0f) {
            left = true;
            face_ = -1;
        }
    };

    const float crowY = floorY(kCrowFloor) + 30.0f;
    bool nearCrow = std::abs(crowY - py_) < 80.0f && std::abs(crowX_ - px_) < 34.0f;
    if (nearCrow && grounded_) {
        float step = crowX_ >= px_ ? -6.0f : 6.0f;
        if (solidAt(floor_, px_ + step)) {
            if (step < 0) left = true;
            else right = true;
            face_ = left ? -1 : 1;
        }
        return;
    }

    if (!grounded_) {
        go(airTargetX_);
        return;
    }

    int targetF = has_ ? 0 : bannerFloor_;
    float targetX = has_ ? kDoor : bannerX_;
    if (floor_ == targetF) {
        go(targetX);
        return;
    }

    if (floor_ < targetF) {
        int nxt = floor_ + 1;
        float gx = gapCenter(nxt);
        bool clear = crowX_ > 210.0f || (std::abs(crowY - py_) > 130.0f && std::abs(crowY - floorY(nxt)) > 130.0f);
        if (std::abs(px_ - gx) > 4.0f) go(gx);
        else if (clear) {
            jump = true;
            airTargetX_ = landX(nxt);
            face_ = airTargetX_ >= px_ ? 1 : -1;
        }
        return;
    }

    int below = floor_ - 1;
    bool clear = crowX_ > 210.0f || (std::abs(crowY - py_) > 130.0f && std::abs(crowY - floorY(below)) > 130.0f);
    if (!clear) return;
    airTargetX_ = landX(below);
    go(airTargetX_);
}

void Game::stepPlay(bool left, bool right, bool jump) {
    playT_ += DT;
    if (stun_ > 0) stun_ -= DT;
    if (inv_ > 0) inv_ -= DT;

    // Perch in the right alcove, then one sweep across the shaft.
    {
        float phase = std::fmod(playT_, 4.6f);
        if (phase < 3.1f) {
            crowX_ = 250.0f;
            crowDir_ = -1.0f;
        } else {
            float u = (phase - 3.1f) / 1.5f;
            crowX_ = 250.0f - u * 176.0f;
            crowDir_ = -1.0f;
        }
    }

    if (stun_ > 0) {
        left = right = jump = false;
    }

    float want = 0;
    if (left && !right) want = -kRun;
    else if (right && !left) want = kRun;
    if (left) face_ = -1;
    else if (right) face_ = 1;
    vx_ = approach(vx_, want, kAccel * DT);

    if (jump && grounded_) {
        grounded_ = false;
        vy_ = kJump;
        blip(520.0f, 0.04f, 0.05f);
    }

    if (!grounded_) vy_ -= kGrav * DT;
    else vy_ = 0;
    vy_ = std::max(vy_, -460.0f);

    float prevY = py_;
    px_ += vx_ * DT;
    py_ += vy_ * DT;
    if (px_ < kL + 6.0f) px_ = kL + 6.0f;
    if (px_ > kR - 6.0f) px_ = kR - 6.0f;

    if (vy_ > 0) {
        for (int i = 0; i < NF; i++) {
            float fy = floorY(i);
            if (prevY < fy - 10.0f && py_ >= fy - 10.0f && solidAt(i, px_)) {
                py_ = fy - 10.0f;
                vy_ = 0;
            }
        }
    }

    grounded_ = false;
    if (vy_ <= 0) {
        for (int i = NF - 1; i >= 0; i--) {
            float fy = floorY(i);
            if (solidAt(i, px_) && prevY >= fy && py_ <= fy) {
                py_ = fy;
                vy_ = 0;
                grounded_ = true;
                floor_ = i;
                break;
            }
        }
    }
    if (py_ < 0) {
        py_ = 0;
        vy_ = 0;
        grounded_ = true;
        floor_ = 0;
    }

    const float crowY = floorY(kCrowFloor) + 26.0f;
    if (std::abs(crowX_ - px_) < 16.0f && std::abs(crowY - (py_ + 22.0f)) < 18.0f) peck();

    if (!has_ && grounded_ && floor_ == bannerFloor_ && std::abs(px_ - bannerX_) < 26.0f && stun_ <= 0) {
        has_ = true;
        blip(720.0f, 0.07f, 0.1f);
        sys_->rumble(0.2f, 0.4f, 80);
        sys_->setLight(150, 40, 30);
    }

    if (has_ && grounded_ && floor_ == 0 && std::abs(px_ - kDoor) < 28.0f) win();
}

void Game::hud(int col, int row, const std::string& s, int pal) {
    if (row < 0 || row > 27) return;
    for (size_t i = 0; i < s.size(); i++) {
        int x = col + int(i);
        unsigned char c = static_cast<unsigned char>(s[i]);
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (x < 0 || x > 39 || c <= 32 || c >= 128) continue;
        sys_->vdp.HUD.set(x, row, gs::entry(art_.font[c - 32], pal));
    }
}

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (h < 1.2f || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::lround(w));
    s.h = int16_t(std::lround(h));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    if (s.x > gs::SCREEN_W + 48 || s.y > gs::SCREEN_H + 48 || s.x + s.w < -48 || s.y + s.h < -48) return;
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
    sys_->vdp.sprite(s);
}

void Game::text(const std::string& s, float x, float y, float scale, int pal, int align) {
    float width = 0;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') width += 8.0f * scale;
        else if (c > 32 && c < 128) width += art_.glyph[c - 32].w * scale + scale;
    }
    if (align == 0) x -= width * 0.5f;
    else if (align > 0) x -= width;
    for (unsigned char c : s) {
        if (c >= 'a' && c <= 'z') c = static_cast<unsigned char>(c - 32);
        if (c == ' ') {
            x += 8.0f * scale;
            continue;
        }
        if (c <= 32 || c >= 128) continue;
        const gs::Mipped& g = art_.glyph[c - 32];
        spr(g, x + g.w * scale * 0.5f, y, float(g.h) * scale, pal, false, false);
        x += g.w * scale + scale;
    }
}

void Game::draw() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    vdp.HUD.clear();

    float want = py_ - 78.0f;
    if (want < -28.0f) want = -28.0f;
    float cap = floorY(NF - 1) + 70.0f - float(gs::SCREEN_H);
    if (want > cap) want = cap;
    camBottom_ = approach(camBottom_, want, 280.0f * DT);
    float cam = camBottom_;
    if (shake_ > 0.02f) cam += std::sin(t_ * 50.0f) * shake_ * 2.5f;

    for (int y = 0; y < gs::SCREEN_H; y++) {
        float wy = cam + float(gs::SCREEN_H - y);
        float u = std::clamp(wy / (floorY(NF - 1) + 40.0f), 0.0f, 1.0f);
        int r = int(1 + u * 2);
        int g = int(1 + u * 2);
        int b = int(3 + u * 5);
        vdp.lineBackdrop[y] = gs::rgb4(r, g, b);
        vdp.lineFog[y] = 0;
    }

    auto sy = [&](float wy) { return float(gs::SCREEN_H) - (wy - cam); };

    for (int i = -1; i < 28; i++) {
        float wy = float(i) * 40.0f;
        spr(art_.wall, 16.0f, sy(wy), 42, PAL_STONE, false, true);
        spr(art_.wall, 304.0f, sy(wy), 42, PAL_STONE, true, true);
    }
    for (int i = 0; i < NF; i++) {
        float y = floorY(i);
        float g0, g1;
        gapOf(i, g0, g1);
        auto span = [&](float a, float b) {
            if (b - a < 8.0f) return;
            for (float x = a + 16.0f; x < b; x += 30.0f) spr(art_.ledge, x, sy(y) + 6.0f, 12, PAL_STONE, false, false);
            spr(art_.ledge, a + 10.0f, sy(y) + 6.0f, 12, PAL_STONE, false, false);
            spr(art_.ledge, b - 10.0f, sy(y) + 6.0f, 12, PAL_STONE, false, false);
        };
        if (g1 <= g0) span(kL, kR);
        else {
            span(kL, g0);
            span(g1, kR);
        }
        if (i % 2 == 0) spr(art_.slit, 28.0f, sy(y + 48.0f), 18, PAL_AMBER, false, false);
        else spr(art_.slit, 292.0f, sy(y + 48.0f), 18, PAL_AMBER, false, false);
    }

    spr(art_.door, kDoor, sy(0), 52, PAL_DOOR, false, true);

    if (!has_) {
        float wave = std::sin(t_ * 3.0f) * 1.5f;
        spr(art_.pole, bannerX_, sy(floorY(bannerFloor_)), 48, PAL_BANNER, false, true);
        spr(art_.banner, bannerX_ + 8.0f + wave, sy(floorY(bannerFloor_) + 30.0f), 34, PAL_BANNER, false, false);
    }

    float crowY = floorY(kCrowFloor) + 26.0f + std::sin(t_ * 6.0f) * 3.0f;
    bool flap = int(t_ * 8.0f) & 1;
    spr(art_.crow[flap ? 1 : 0], crowX_, sy(crowY), 20, PAL_CROW, crowDir_ < 0, false);

    if (inv_ <= 0 || (int(t_ * 18.0f) & 1)) {
        spr(hero(), px_, sy(py_), 58, PAL_HERO, face_ < 0, true);
        if (has_) spr(art_.banner, px_ + face_ * 12.0f, sy(py_ + 40.0f), 28, PAL_BANNER, face_ < 0, false);
    }

    std::string hearts;
    for (int i = 0; i < lives_; i++) hearts += "O ";
    hud(1, 1, hearts, PAL_HUD);
    hud(12, 1, has_ ? "BANNER" : "CLIMB", has_ ? PAL_BANNER : PAL_HUD);
    char fl[16];
    std::snprintf(fl, sizeof(fl), "FL %d", floor_ + 1);
    hud(32, 1, fl, PAL_HUD);

    if (mode_ == Mode::Title) {
        text("TOWER BANN", 160, 58, 1.15f, PAL_HUD, 0);
        text("ONE TOWER", 160, 88, 0.55f, PAL_AMBER, 0);
        text("BRING THE BANNER BACK", 160, 108, 0.48f, PAL_BANNER, 0);
        text("THEN IT IS DONE", 160, 126, 0.48f, PAL_STONE, 0);
        if ((int(t_ * 2.0f) & 1) == 0) text("PRESS START", 160, 156, 0.65f, PAL_HUD, 0);
    } else if (mode_ == Mode::Pause) {
        text("PAUSED", 160, 96, 1.0f, PAL_HUD, 0);
    } else if (mode_ == Mode::Victory) {
        text("THE BANNER IS BACK", 160, 78, 0.62f, PAL_BANNER, 0);
        text("THEN IT IS DONE", 160, 104, 0.62f, PAL_HUD, 0);
    } else if (mode_ == Mode::Over) {
        text("THE TOWER KEEPS IT", 160, 80, 0.58f, PAL_HUD, 0);
        text("START TO TRY AGAIN", 160, 108, 0.48f, PAL_AMBER, 0);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    resetRun();
    mode_ = Mode::Title;
    over_ = false;
    won_ = false;
    titleHold_ = 0;
    t_ = 0;
    camBottom_ = -28.0f;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += DT;
    if (shake_ > 0) shake_ = std::max(0.0f, shake_ - DT);
    if (beep_ > 0) {
        beep_ -= DT;
        if (beep_ <= 0) sys.apu.tone(0, 0, 0);
    }

    const gs::Pad& pad = sys.pad;
    bool left = pad.down(gs::BTN_LEFT) || pad.axisX < -0.35f;
    bool right = pad.down(gs::BTN_RIGHT) || pad.axisX > 0.35f;
    bool jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B) || pad.pressed(gs::BTN_C);
    bool start = pad.pressed(gs::BTN_START);

    if (mode_ == Mode::Title) {
        titleHold_++;
        if (bot_ && titleHold_ > 8) begin();
        else if (start || pad.pressed(gs::BTN_A)) begin();
    } else if (mode_ == Mode::Pause) {
        if (start) mode_ = Mode::Play;
    } else if (mode_ == Mode::Play) {
        if (bot_) bot(left, right, jump);
        else if (start) {
            mode_ = Mode::Pause;
            vx_ = 0;
        }
        if (mode_ == Mode::Play) stepPlay(left, right, jump);
    } else if (mode_ == Mode::Over || mode_ == Mode::Victory) {
        if (!bot_ && start) {
            mode_ = Mode::Title;
            titleHold_ = 0;
            over_ = false;
            won_ = false;
            resetRun();
        }
    }

    draw();
}

}  // namespace towerbann

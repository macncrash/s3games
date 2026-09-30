#include "game/ladder.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace culvert {
namespace {

constexpr float kDt = 1.f / 60.f;
constexpr float kHw = 7.f;
constexpr float kPh = 26.f;
constexpr float kRun = 124.f;
constexpr float kJump = -268.f;
constexpr float kGrav = 720.f;
constexpr float kWorld = 1680.f;
constexpr float kWater = 206.f;
constexpr float kLadderX = 1548.f;
constexpr float kLadderTop = 42.f;
constexpr float kLimit = 48.f;

}  // namespace

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt(sys.vdp, art_);
    sys.vdp.A.enabled = false;
    sys.vdp.B.enabled = true;
    sys.vdp.hudEnabled = true;
    sys.vdp.setFogColor(gs::rgb4(2, 2, 3));
    sys.apu.setMaster(0.4f);
    bootWall();
    mode_ = Mode::Title;
}

void Game::bootWall() {
    solids_.clear();
    const float deck[][3] = {
        {0.f, 168.f, 300.f},   {372.f, 168.f, 210.f}, {650.f, 146.f, 190.f}, {908.f, 168.f, 180.f},
        {1132.f, 138.f, 174.f}, {1372.f, 168.f, 308.f},
    };
    for (auto& d : deck) solids_.push_back({d[0], d[1], d[2], 80.f});
}

void Game::begin() {
    px_ = 48.f;
    py_ = 168.f;
    vx_ = vy_ = 0.f;
    cam_ = 0.f;
    clock_ = 0.f;
    t_ = 0.f;
    grounded_ = true;
    climbing_ = false;
    rungs_ = 0;
    won_ = false;
    over_ = false;
    reason_ = "THE FAR LADDER IS STILL AHEAD";
    mode_ = Mode::Play;
    blip(520.f);
}

int Game::marker() const {
    if (mode_ == Mode::Title || mode_ == Mode::Pause) return 0;
    if (mode_ == Mode::Victory) return 2;
    if (mode_ == Mode::Fail) return 3;
    return 1;
}

bool Game::hits(float l, float t, float r, float b, const Solid& s) const {
    return r > s.x && l < s.x + s.w && b > s.y && t < s.y + s.h;
}

void Game::bot(bool& left, bool& right, bool& jump, bool& up) {
    left = right = jump = up = false;
    if (px_ > kLadderX - 8.f && px_ < kLadderX + 18.f && py_ > kLadderTop) {
        up = true;
        if (px_ > kLadderX + 3.f) left = true;
        else if (px_ < kLadderX - 3.f) right = true;
        return;
    }
    right = true;
    const float probe = px_ + 14.f;
    bool floorAhead = false;
    bool wall = false;
    for (const Solid& s : solids_) {
        if (probe > s.x + 1.f && probe < s.x + s.w - 1.f && s.y >= py_ - 6.f && s.y <= py_ + 36.f) floorAhead = true;
        if (px_ + kHw + 2.f > s.x && px_ + kHw + 2.f < s.x + 10.f && py_ - 4.f > s.y && py_ - kPh < s.y + s.h) wall = true;
    }
    if (grounded_ && (!floorAhead || wall)) jump = true;
}

void Game::update() {
    bool left = false, right = false, jump = false, up = false;
    const gs::Pad& pad = sys_->pad;
    if (bot_) bot(left, right, jump, up);
    else {
        left = pad.down(gs::BTN_LEFT);
        right = pad.down(gs::BTN_RIGHT);
        jump = pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C) || pad.pressed(gs::BTN_B);
        up = pad.down(gs::BTN_UP);
        if (std::fabs(pad.axisX) > 0.3f) {
            if (pad.axisX > 0) right = true;
            else left = true;
        }
    }

    const bool onLadder = px_ > kLadderX - 12.f && px_ < kLadderX + 16.f && py_ <= 172.f && py_ >= kLadderTop - 2.f;
    if (onLadder && up && py_ > kLadderTop) {
        climbing_ = true;
        vx_ = 0.f;
        vy_ = 0.f;
        px_ += (kLadderX - px_) * 0.35f;
        py_ -= 62.f * kDt;
        if (py_ < 168.f) rungs_ = std::max(rungs_, int((168.f - py_) / 16.f));
        grounded_ = false;
        if (py_ <= kLadderTop) {
            py_ = kLadderTop;
            win();
            return;
        }
    } else {
        climbing_ = false;
        float accel = bot_ && px_ > kLadderX - 56.f ? 70.f : kRun;
        if (left) vx_ -= accel * 6.f * kDt;
        if (right) vx_ += accel * 6.f * kDt;
        if (!left && !right) vx_ *= 0.72f;
        vx_ = std::clamp(vx_, -accel, accel);
        if (std::fabs(vx_) > 8.f) face_ = vx_ > 0 ? 1.f : -1.f;
        if (jump && grounded_) {
            vy_ = kJump;
            grounded_ = false;
            blip(340.f);
        }
        vy_ += kGrav * kDt;
        px_ += vx_ * kDt;
        for (const Solid& s : solids_) {
            if (!hits(px_ - kHw, py_ - kPh, px_ + kHw, py_, s)) continue;
            if (vx_ > 0) px_ = s.x - kHw - 0.05f;
            else if (vx_ < 0) px_ = s.x + s.w + kHw + 0.05f;
            vx_ = 0.f;
        }
        px_ = std::clamp(px_, kHw, kWorld - kHw);
        py_ += vy_ * kDt;
        grounded_ = false;
        for (const Solid& s : solids_) {
            if (!hits(px_ - kHw, py_ - kPh, px_ + kHw, py_, s)) continue;
            if (vy_ >= 0) {
                py_ = s.y;
                vy_ = 0.f;
                grounded_ = true;
            } else {
                py_ = s.y + s.h + kPh;
                vy_ = 0.f;
            }
        }
        if (!grounded_ && py_ > kWater) {
            lose("THE WATER TAKES THE CULVERT");
            return;
        }
    }

    clock_ += kDt;
    if (clock_ > kLimit) {
        lose("THE LADDER STAYS OUT OF REACH");
        return;
    }
    float want = std::clamp(px_ - 120.f, 0.f, kWorld - float(gs::SCREEN_W));
    cam_ += (want - cam_) * 0.18f;
}

void Game::win() {
    won_ = true;
    reason_ = "THE FAR LADDER IS REACHED";
    mode_ = Mode::Victory;
    hold_ = 1.2f;
    sys_->apu.tone(0, 523.f, 0.18f);
    sys_->apu.tone(1, 659.f, 0.16f);
    sys_->apu.tone(2, 784.f, 0.14f);
}

void Game::lose(const char* why) {
    won_ = false;
    reason_ = why;
    mode_ = Mode::Fail;
    hold_ = 1.4f;
    sys_->apu.noiseBurst(0.22f, 280.f, 0.25f);
}

void Game::blip(float freq) {
    sys_->apu.tone(0, freq, 0.1f);
    blip_ = 0.07f;
}

void Game::serviceAudio() {
    if (blip_ > 0.f) {
        blip_ -= kDt;
        if (blip_ <= 0.f) sys_->apu.tone(0, 0.f, 0.f);
    }
    if (mode_ == Mode::Play && !climbing_) {
        float drip = 90.f + 40.f * std::sin(t_ * 3.f);
        sys_->apu.tone(2, drip, 0.015f);
    } else if (mode_ != Mode::Victory) {
        sys_->apu.tone(2, 0.f, 0.f);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    t_ += kDt;
    const gs::Pad& pad = sys.pad;
    if (mode_ == Mode::Title) {
        if (bot_ || pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_C)) begin();
    } else if (mode_ == Mode::Play) {
        if (!bot_ && pad.pressed(gs::BTN_START)) mode_ = Mode::Pause;
        else update();
    } else if (mode_ == Mode::Pause) {
        if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) mode_ = Mode::Play;
    } else {
        hold_ -= kDt;
        if (hold_ <= 0.f) over_ = true;
    }
    serviceAudio();
    draw();
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

void Game::spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip, bool feet) {
    if (!(h > 1.5f) || m.h < 1) return;
    float w = h * float(m.w) / float(m.h);
    gs::Sprite s;
    s.w = int16_t(std::clamp(int(std::lround(w)), 1, 2000));
    s.h = int16_t(std::clamp(int(std::lround(h)), 1, 2000));
    s.x = int16_t(std::lround(cx - s.w * 0.5f));
    s.y = int16_t(std::lround(feet ? cy - s.h : cy - s.h * 0.5f));
    s.img = m.pick(h);
    s.pal = uint8_t(pal);
    s.hflip = flip;
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
        spr(g, x + gw * 0.5f, y, float(g.h) * scale, pal, false, false);
        x += gw;
    }
}

void Game::draw() {
    gs::VDP& v = sys_->vdp;
    v.clearSprites();
    v.HUD.clear();
    v.B.scroll(-int(std::lround(cam_ * 0.35f)), 0);
    const bool bad = mode_ == Mode::Fail;
    for (int y = 0; y < gs::SCREEN_H; ++y) {
        v.road[y].on = false;
        v.lineFog[y] = uint8_t(y > 180 ? (y - 180) / 4 : 0);
        if (y < 28) v.lineBackdrop[y] = gs::rgb4(2, 2, 3);
        else if (y < 176) {
            int g = 3 + (y / 40);
            v.lineBackdrop[y] = bad ? gs::rgb4(6, 2, 2) : gs::rgb4(g, g - 1, g - 1);
        } else {
            int bob = int(2.f * std::sin(t_ * 2.f + y * 0.2f));
            v.lineBackdrop[y] = gs::rgb4(1, 3 + bob, 6);
        }
    }

    auto world = [&](float wx, float wy, const gs::Mipped& m, float h, int pal, bool flip, bool feet) {
        float sx = wx - cam_;
        if (sx < -80.f || sx > gs::SCREEN_W + 80.f) return;
        spr(m, sx, wy, h, pal, flip, feet);
    };

    if (mode_ == Mode::Title || mode_ == Mode::Victory || mode_ == Mode::Fail) {
        const char* head = mode_ == Mode::Victory ? "FAR LADDER" : mode_ == Mode::Fail ? "LOST THE CULVERT" : "CULVERT";
        int pal = mode_ == Mode::Fail ? PAL_ALERT : mode_ == Mode::Victory ? PAL_GOOD : PAL_AMBER;
        text(head, 160.f, 36.f, 1.15f, pal);
        text(mode_ == Mode::Title ? "REACH THE FAR LADDER" : reason_, 160.f, 64.f, 0.55f, PAL_TEXT);
        if (mode_ == Mode::Title) text("START", 160.f, 96.f, 0.5f, PAL_GOOD);
    }

    const bool showMan = mode_ != Mode::Title;
    if (showMan) {
        bool stride = grounded_ && std::fabs(vx_) > 12.f && int(t_ * 10.f) % 2 == 0;
        world(px_, py_, stride ? art_.stride : art_.man, 30.f, PAL_MAN, face_ < 0.f, true);
        world(px_ + face_ * 8.f, py_ - 20.f, art_.lamp, 8.f, PAL_LAMP, false, false);
    } else {
        spr(art_.man, 70.f, 168.f, 34.f, PAL_MAN, false, true);
        spr(art_.lamp, 80.f, 146.f, 8.f, PAL_LAMP, false, false);
        spr(art_.rung, 250.f, 150.f, 70.f, PAL_LADDER, false, true);
    }

    for (float y = 36.f; y < 168.f; y += 28.f) world(kLadderX, y + 28.f, art_.rung, 28.f, PAL_LADDER, false, true);
    for (const Solid& s : solids_) {
        for (float x = s.x; x < s.x + s.w - 4.f; x += 32.f) world(x + 16.f, s.y + 8.f, art_.brick, 16.f, PAL_BRICK, false, false);
    }
    for (float x = 20.f; x < kWorld; x += 96.f) world(x, 52.f, art_.arch, 26.f, PAL_IRON, false, false);
    for (int i = 0; i < 5; ++i) {
        float x = 180.f + i * 280.f;
        float y = 70.f + std::fmod(t_ * 28.f + i * 17.f, 90.f);
        world(x, y, art_.drip, 12.f, PAL_WATER, false, false);
    }

    if (mode_ == Mode::Play || mode_ == Mode::Pause) {
        int left = std::max(0, int((kLadderX - px_) / 16.f));
        hud(1, 1, "FAR LADDER", PAL_AMBER);
        hud(28, 1, std::to_string(left) + " PACES", PAL_TEXT);
        hud(1, 26, "ARROWS MOVE   A JUMP   UP CLIMB", PAL_TEXT);
        if (mode_ == Mode::Pause) hudC(12, "PAUSED", PAL_AMBER);
    } else if (mode_ == Mode::Victory) {
        hudC(24, "THE FAR LADDER IS REACHED", PAL_GOOD);
    } else if (mode_ == Mode::Fail) {
        hudC(24, reason_, PAL_ALERT);
    } else {
        hudC(26, "ANYTHING ELSE IS A LOSS", PAL_TEXT);
    }
}

}  // namespace culvert

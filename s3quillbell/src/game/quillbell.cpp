#include "game/quillbell.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

#include "console/gfx.h"

namespace quillbell {

namespace {

struct Node {
    float x, y;
};

// A desk flourish that ends under the bell. The last node rings it.
constexpr Node kPath[] = {
    {64, 156}, {96, 128}, {132, 108}, {168, 136}, {198, 96}, {232, 118}, {258, 86}, {286, 132},
};
constexpr int kNodes = int(sizeof(kPath) / sizeof(kPath[0]));
constexpr int kTries = 3;

struct Sample {
    float x, y;
    int seg;
};

std::vector<Sample> stroke() {
    std::vector<Sample> s;
    for (int i = 1; i < kNodes; i++) {
        float dx = kPath[i].x - kPath[i - 1].x;
        float dy = kPath[i].y - kPath[i - 1].y;
        float len = std::sqrt(dx * dx + dy * dy);
        int n = std::max(1, int(len / 6.f));
        for (int k = 1; k <= n; k++) {
            float t = float(k) / float(n);
            s.push_back({kPath[i - 1].x + dx * t, kPath[i - 1].y + dy * t, i});
        }
    }
    return s;
}

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(p * 16 + i++, c);
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

gs::Bitmap quillBitmap() {
    gs::Bitmap b(22, 52);
    b.ellipse(8, 14, 7, 13, 3);
    b.ellipse(7, 13, 3, 8, 2);
    for (int i = 0; i < 6; i++) {
        float y = 7.f + float(i) * 3.4f;
        b.line(8, y, 2, y + 2, 3, 1.f);
    }
    b.line(10, 8, 13, 48, 1, 2.2f);
    b.line(13, 40, 16, 50, 4, 1.5f);
    b.set(15, 49, 5);
    return b;
}

gs::Bitmap wellBitmap() {
    gs::Bitmap b(26, 20);
    b.ellipse(13, 11, 11, 7, 1);
    b.ellipse(13, 10, 7, 4, 2);
    b.ellipse(13, 10, 3, 2, 3);
    return b;
}

gs::Bitmap blotBitmap() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.2f, 2.2f, 2.f, 2.f, 1);
    return b;
}

gs::Bitmap bellBitmap() {
    gs::Bitmap b(28, 30);
    b.ellipse(14, 16, 12, 11, 1);
    b.ellipse(14, 14, 7, 6, 2);
    b.rect(6, 22, 16, 3, 3);
    b.rect(12, 2, 4, 6, 3);
    b.set(14, 4, 4);
    return b;
}

gs::Bitmap clapperBitmap() {
    gs::Bitmap b(6, 8);
    b.line(3, 0, 3, 4, 1, 1.f);
    b.ellipse(3, 6, 2, 2, 2);
    return b;
}

gs::Image words(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

float dist2(float ax, float ay, float bx, float by) {
    float dx = bx - ax;
    float dy = by - ay;
    return dx * dx + dy * dy;
}

}  // namespace

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    pal(vdp, 0, {0, gs::rgb4(14, 12, 8)});
    pal(vdp, 1, {0, gs::rgb4(1, 1, 3), gs::rgb4(4, 3, 8)});
    pal(vdp, 2, {0, gs::rgb4(5, 3, 1), gs::rgb4(11, 8, 4), gs::rgb4(15, 13, 9), gs::rgb4(13, 10, 3), gs::rgb4(2, 1, 1)});
    pal(vdp, 3, {0, gs::rgb4(8, 7, 6)});
    pal(vdp, 4, {0, gs::rgb4(14, 12, 8), gs::rgb4(10, 8, 5), gs::rgb4(4, 3, 2), gs::rgb4(12, 10, 6), gs::rgb4(11, 2, 2),
                 gs::rgb4(14, 6, 3)});
    pal(vdp, 5, {0, gs::rgb4(3, 2, 1), gs::rgb4(1, 1, 2), gs::rgb4(8, 6, 3)});
    pal(vdp, 6, {0, gs::rgb4(15, 13, 5), gs::rgb4(12, 9, 2), gs::rgb4(8, 6, 2), gs::rgb4(15, 15, 10)});
    pal(vdp, 7, {0, gs::rgb4(6, 8, 4)});

    quill_ = gs::uploadImage(vdp, quillBitmap());
    well_ = gs::uploadImage(vdp, wellBitmap());
    blot_ = gs::uploadImage(vdp, blotBitmap());
    bell_ = gs::uploadImage(vdp, bellBitmap());
    clapper_ = gs::uploadImage(vdp, clapperBitmap());
    title_ = words(vdp, "S3 QUILLBELL", 2);
    hint_ = words(vdp, "RING IT BEFORE TRY 3 DIES", 1);
    start_ = words(vdp, "PRESS START", 1);
    holdImg_ = words(vdp, "HOLD A  STEER THE NIB", 1);
    rung_ = words(vdp, "BELL RUNG", 2);
    died_ = words(vdp, "THIRD TRY DIED", 2);
    line_ = words(vdp, "THE BELL RINGS BEFORE THE THIRD TRY DIES", 1);
    for (int d = 0; d < 10; d++) digit_[d] = words(vdp, std::string(1, char('0' + d)).c_str(), 1);

    gs::Bitmap desk(256, 176);
    desk.rect(0, 0, 256, 176, 3);
    desk.rect(6, 6, 244, 164, 1);
    uint32_t rng = 0xB311u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 16) & 255;
    };
    for (int i = 0; i < 70; i++) {
        int x = 10 + int(rnd() % 230);
        int y = 12 + int(rnd() % 150);
        desk.set(x, y, 2);
    }
    for (int y = 36; y < 158; y += 18) desk.rect(18, float(y), 220, 1, 4);
    desk.rect(18, 24, 2, 130, 5);

    gs::TileAlloc tiles(vdp, 1);
    vdp.A.clear();
    vdp.B.clear();
    vdp.B.enabled = false;
    vdp.HUD.clear();
    gs::bitmapToPlane(tiles, vdp.A, 4, 3, desk, 4);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int v = y < 40 ? 1 : 0;
        vdp.lineBackdrop[y] = gs::rgb4(2 + v, 2, 3);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    marks_ = kNodes;
    seals_ = 1;
    dead_ = 0;
    x_ = kPath[0].x;
    y_ = kPath[0].y;
    why_ = "the bell rings before the third try dies";
    buildArt();
    sys.apu.setMaster(0.75f);
    drawScene();
}

void Game::spr(const gs::Image& im, float x, float y, int pal, bool nib) {
    if (!im.w) return;
    gs::Sprite s;
    s.img = im;
    s.w = int16_t(im.w);
    s.h = int16_t(im.h);
    s.x = int16_t(std::lround(x - im.w * 0.5f));
    s.y = int16_t(nib ? std::lround(y - im.h) : std::lround(y - im.h * 0.5f));
    s.pal = uint8_t(pal);
    sys_->vdp.sprite(s);
}

void Game::beginTry() {
    seals_ = 1;
    x_ = kPath[0].x;
    y_ = kPath[0].y;
    ink_ = false;
    wasInk_ = false;
    mode_ = Mode::Ink;
    why_ = "hold the stroke";
}

void Game::dieTry(const char* why) {
    if (mode_ != Mode::Ink) return;
    dead_++;
    ink_ = false;
    wasInk_ = false;
    why_ = why;
    sys_->apu.tone(0, 98.f, 0.1f);
    sys_->apu.noiseBurst(0.16f, 320.f, 0.16f);
    sys_->rumble(0.3f, 0.08f, 70);
    sys_->setLight(48, 18, 24);
    if (dead_ >= kTries) {
        mode_ = Mode::Lose;
        won_ = false;
        over_ = false;
        hold_ = 50;
        why_ = "the third try died";
        return;
    }
    mode_ = Mode::Gap;
    hold_ = 36;
}

void Game::ring() {
    mode_ = Mode::Ring;
    won_ = true;
    ink_ = true;
    swing_ = 10.f;
    chime_ = 0;
    hold_ = 100;
    why_ = "the bell rings before the third try dies";
    sys_->setLight(220, 180, 60);
    sys_->rumble(0.2f, 0.5f, 180);
}

void Game::drawScene() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    static const std::vector<Sample> samples = stroke();

    float bx = 40.f + std::sin(swing_) * 7.f;
    spr(bell_, bx, 46, 6);
    spr(clapper_, bx + std::sin(swing_ * 1.4f) * 4.f, 58, 6);
    spr(well_, 48, 186, 5);

    for (const Sample& s : samples) {
        bool wet = mode_ != Mode::Title && s.seg < seals_;
        spr(blot_, s.x, s.y, wet ? 1 : 3);
    }

    if (mode_ != Mode::Title) {
        float lift = ink_ ? 0.f : 8.f;
        spr(quill_, x_, y_ - lift, 2, true);
    } else {
        spr(quill_, kPath[0].x, kPath[0].y - 8.f, 2, true);
    }

    for (int i = 0; i < kTries; i++) {
        spr(blot_, 250.f + float(i) * 12.f, 18.f, i < dead_ ? 4 : 7);
    }

    if (mode_ == Mode::Title) {
        spr(title_, 168, 26, 6);
        spr(hint_, 168, 48, 0);
        spr(start_, 160, 204, 0);
    } else if (mode_ == Mode::Ring) {
        spr(rung_, 168, 22, 6);
        spr(line_, 160, 204, 0);
    } else if (mode_ == Mode::Lose) {
        spr(died_, 160, 22, 0);
    } else {
        spr(digit_[seals_], 292, 34, 0);
        if (!ink_ && mode_ == Mode::Ink) spr(holdImg_, 160, 204, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;

    if (swing_ > 0.15f) swing_ *= 0.96f;
    else if (mode_ != Mode::Ring) swing_ = 0;

    if (mode_ == Mode::Title) {
        bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_ && sys.frame > 18) start = true;
        if (start) {
            dead_ = 0;
            won_ = false;
            over_ = false;
            beginTry();
            sys.apu.noiseBurst(0.08f, 1400.f, 0.05f);
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Gap) {
        if (hold_ > 0) hold_--;
        if (hold_ == 0) beginTry();
        sys.apu.tone(0, 0, 0);
        drawScene();
        return;
    }

    if (mode_ == Mode::Lose) {
        if (hold_ > 0) hold_--;
        else over_ = true;
        if (!bot_ && pad.pressed(gs::BTN_START)) {
            dead_ = 0;
            won_ = false;
            over_ = false;
            beginTry();
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Ring) {
        swing_ = 8.f * std::sin(float(chime_) * 0.45f);
        if (chime_ == 4) sys.apu.tone(0, 784.f, 0.16f);
        if (chime_ == 18) sys.apu.tone(1, 988.f, 0.14f);
        if (chime_ == 34) sys.apu.tone(2, 1174.f, 0.12f);
        if (chime_ == 70) {
            sys.apu.tone(0, 0, 0);
            sys.apu.tone(1, 0, 0);
            sys.apu.tone(2, 0, 0);
        }
        chime_++;
        if (hold_ > 0) hold_--;
        else over_ = true;
        if (!bot_ && over_ && pad.pressed(gs::BTN_START)) {
            dead_ = 0;
            won_ = false;
            over_ = false;
            swing_ = 0;
            beginTry();
        }
        drawScene();
        return;
    }

    float ax = 0, ay = 0;
    bool pressInk = false;
    bool botBreak = false;
    if (bot_) {
        // The first try lifts early so a dead try is on the page. The next one finishes.
        if (dead_ == 0 && seals_ >= 3) {
            botBreak = true;
        } else if (seals_ < kNodes) {
            ax = kPath[seals_].x - x_;
            ay = kPath[seals_].y - y_;
            pressInk = true;
        }
    } else {
        if (pad.down(gs::BTN_LEFT)) ax -= 1;
        if (pad.down(gs::BTN_RIGHT)) ax += 1;
        if (pad.down(gs::BTN_UP)) ay -= 1;
        if (pad.down(gs::BTN_DOWN)) ay += 1;
        if (std::fabs(pad.axisX) > 0.2f) ax = pad.axisX;
        if (std::fabs(pad.axisY) > 0.2f) ay = -pad.axisY;
        pressInk = pad.down(gs::BTN_A) || pad.down(gs::BTN_B);
    }

    if (botBreak) {
        dieTry("the ink broke");
        drawScene();
        return;
    }

    ink_ = pressInk;
    float speed = bot_ ? 3.4f : 2.3f;
    float mag = std::sqrt(ax * ax + ay * ay);
    if (!bot_ && mag > 1.f) {
        ax /= mag;
        ay /= mag;
        mag = 1.f;
    }
    if (mag > 0.01f && ink_) {
        float step = bot_ ? std::min(speed, mag) : speed * std::min(mag, 1.f);
        float ux = ax / mag;
        float uy = ay / mag;
        x_ = std::clamp(x_ + ux * step, 36.f, 304.f);
        y_ = std::clamp(y_ + uy * step, 64.f, 200.f);
    }

    if (wasInk_ && !ink_ && seals_ > 1 && seals_ < kNodes) {
        dieTry("the ink broke");
        drawScene();
        return;
    }
    wasInk_ = ink_;

    if (ink_ && seals_ < kNodes) {
        float d2 = dist2(x_, y_, kPath[seals_].x, kPath[seals_].y);
        float back = dist2(x_, y_, kPath[seals_ - 1].x, kPath[seals_ - 1].y);
        if (d2 > 64.f * 64.f && back > 28.f * 28.f) {
            dieTry("the stroke left the page");
            drawScene();
            return;
        }
        if (d2 < 13.f * 13.f) {
            x_ = kPath[seals_].x;
            y_ = kPath[seals_].y;
            seals_++;
            sys.apu.noiseBurst(0.04f, 1100.f, 0.03f);
            if (seals_ >= kNodes) ring();
        }
    }

    bool moving = mag > 0.01f && ink_ && mode_ == Mode::Ink;
    if (moving) {
        scratch_ += 0.4f;
        sys.apu.tone(0, 130.f + 22.f * std::sin(scratch_), 0.03f);
    } else if (mode_ == Mode::Ink) {
        sys.apu.tone(0, 0, 0);
    }

    drawScene();
}

}  // namespace quillbell

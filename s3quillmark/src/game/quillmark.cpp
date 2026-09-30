#include "game/quillmark.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>
#include <vector>

#include "console/gfx.h"

namespace quillmark {

namespace {

struct Node {
    float x, y;
};

// One flourish: a loop, then the tail. The last node finishes the mark.
constexpr Node kPath[] = {
    {78, 86},  {98, 112},  {128, 138}, {166, 148}, {206, 134}, {232, 106}, {226, 74},
    {192, 58}, {154, 66},  {128, 94},  {146, 122}, {186, 130}, {226, 116}, {262, 88},
};
constexpr int kNodes = int(sizeof(kPath) / sizeof(kPath[0]));

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
        int n = std::max(1, int(len / 5.f));
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
    gs::Bitmap b(24, 56);
    b.ellipse(9, 16, 8, 15, 3);
    b.ellipse(8, 15, 4, 9, 2);
    b.line(10, 6, 8, 28, 2, 1.2f);
    for (int i = 0; i < 7; i++) {
        float y = 8.f + float(i) * 3.2f;
        b.line(9, y, 2, y + 3, 3, 1.f);
    }
    b.line(11, 10, 14, 50, 1, 2.4f);
    b.line(14, 42, 16, 54, 4, 1.6f);
    b.line(13, 46, 15, 54, 5, 1.f);
    b.set(15, 53, 5);
    return b;
}

gs::Bitmap wellBitmap() {
    gs::Bitmap b(28, 22);
    b.ellipse(14, 12, 12, 8, 1);
    b.ellipse(14, 10, 8, 5, 2);
    b.ellipse(14, 10, 4, 2, 3);
    b.rect(6, 4, 16, 4, 1);
    return b;
}

gs::Bitmap blotBitmap() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.2f, 2.2f, 2.1f, 2.1f, 1);
    b.set(2, 2, 2);
    return b;
}

gs::Image words(gs::VDP& vdp, const char* s, int scale) {
    gs::TextStyle st;
    st.scale = scale;
    st.color = 1;
    st.spacing = 1;
    return gs::uploadImage(vdp, gs::textBitmap(s, st));
}

}  // namespace

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    pal(vdp, 0, {0, gs::rgb4(15, 14, 12)});
    pal(vdp, 1, {0, gs::rgb4(1, 1, 2), gs::rgb4(3, 3, 6)});
    pal(vdp, 2, {0, gs::rgb4(4, 2, 1), gs::rgb4(10, 8, 5), gs::rgb4(14, 13, 10), gs::rgb4(14, 12, 5), gs::rgb4(1, 1, 2)});
    pal(vdp, 3, {0, gs::rgb4(9, 8, 7)});
    pal(vdp, 4, {0, gs::rgb4(15, 14, 11), gs::rgb4(13, 11, 8), gs::rgb4(6, 4, 2), gs::rgb4(12, 11, 8), gs::rgb4(12, 3, 2),
                 gs::rgb4(15, 8, 5)});
    pal(vdp, 5, {0, gs::rgb4(2, 1, 1), gs::rgb4(1, 1, 3), gs::rgb4(6, 5, 8)});
    pal(vdp, 6, {0, gs::rgb4(15, 12, 4)});

    quill_ = gs::uploadImage(vdp, quillBitmap());
    well_ = gs::uploadImage(vdp, wellBitmap());
    blot_ = gs::uploadImage(vdp, blotBitmap());
    title_ = words(vdp, "S3 QUILLMARK", 2);
    hint_ = words(vdp, "TRACE THE MARK", 1);
    start_ = words(vdp, "PRESS START", 1);
    holdImg_ = words(vdp, "HOLD A TO INK", 1);
    done_ = words(vdp, "FINISHED MARK", 2);
    ends_ = words(vdp, "A FINISHED MARK ENDS IT", 1);
    slash_ = words(vdp, "/", 1);
    for (int d = 0; d < 10; d++) digit_[d] = words(vdp, std::string(1, char('0' + d)).c_str(), 1);

    gs::Bitmap paper(256, 176);
    paper.rect(0, 0, 256, 176, 3);
    paper.rect(4, 4, 248, 168, 1);
    uint32_t rng = 0x51A7u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 16) & 255;
    };
    for (int i = 0; i < 90; i++) {
        int x = 8 + int(rnd() % 236);
        int y = 8 + int(rnd() % 156);
        paper.set(x, y, 2);
        if (rnd() & 1) paper.set(x + 1, y, 2);
    }
    for (int y = 28; y < 160; y += 16) paper.rect(16, float(y), 224, 1, 4);
    paper.ellipse(226, 24, 14, 11, 5);
    paper.ellipse(223, 21, 5, 4, 6);

    gs::TileAlloc tiles(vdp, 1);
    vdp.A.clear();
    vdp.B.clear();
    vdp.B.enabled = false;
    vdp.HUD.clear();
    gs::bitmapToPlane(tiles, vdp.A, 4, 3, paper, 4);

    for (int y = 0; y < gs::SCREEN_H; y++) {
        int v = 2 + y / 40;
        vdp.lineBackdrop[y] = gs::rgb4(3 + v, 2, 1);
    }
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    marks_ = kNodes;
    seals_ = 1;
    x_ = kPath[0].x;
    y_ = kPath[0].y;
    buildArt();
    sys.apu.setMaster(0.8f);
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

void Game::drawScene() {
    gs::VDP& vdp = sys_->vdp;
    vdp.clearSprites();
    static const std::vector<Sample> samples = stroke();

    spr(well_, 52, 178, 5);

    if (mode_ != Mode::Title) {
        for (const Sample& s : samples) {
            bool wet = s.seg < seals_;
            spr(blot_, s.x, s.y, wet ? 1 : 3);
        }
    } else {
        for (const Sample& s : samples) spr(blot_, s.x, s.y, 3);
    }

    float lift = ink_ ? 0.f : 7.f;
    spr(quill_, x_, y_ - lift, 2, true);

    if (mode_ == Mode::Title) {
        spr(title_, 160, 28, 6);
        spr(hint_, 160, 52, 0);
        spr(start_, 160, 198, 0);
    } else if (mode_ == Mode::Done) {
        spr(done_, 160, 22, 6);
        spr(ends_, 160, 44, 0);
    } else {
        int a = seals_;
        int b = marks_;
        if (a > 9) spr(digit_[a / 10], 250, 14, 0);
        spr(digit_[a % 10], 258, 14, 0);
        spr(slash_, 266, 14, 0);
        if (b > 9) spr(digit_[b / 10], 274, 14, 0);
        spr(digit_[b % 10], 282, 14, 0);
        if (!ink_) spr(holdImg_, 160, 200, 0);
    }
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;
    bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);

    if (mode_ == Mode::Title) {
        if (bot_ && sys.frame > 20) start = true;
        if (start) {
            mode_ = Mode::Trace;
            seals_ = 1;
            x_ = kPath[0].x;
            y_ = kPath[0].y;
            sys.apu.noiseBurst(0.12f, 1800.f, 0.08f);
        }
        ink_ = false;
        drawScene();
        return;
    }

    if (mode_ == Mode::Done) {
        ink_ = true;
        if (hold_ > 0) {
            hold_--;
            if (hold_ == 70) {
                sys.apu.tone(0, 523.f, 0.12f);
                sys.apu.tone(1, 659.f, 0.1f);
                sys.apu.tone(2, 784.f, 0.08f);
            }
            if (hold_ == 20) {
                sys.apu.tone(0, 0, 0);
                sys.apu.tone(1, 0, 0);
                sys.apu.tone(2, 0, 0);
            }
            if (hold_ == 0) over_ = true;
        }
        drawScene();
        return;
    }

    float ax = 0, ay = 0;
    bool pressInk = false;
    if (bot_) {
        ax = kPath[seals_].x - x_;
        ay = kPath[seals_].y - y_;
        pressInk = true;
    } else {
        if (pad.down(gs::BTN_LEFT)) ax -= 1;
        if (pad.down(gs::BTN_RIGHT)) ax += 1;
        if (pad.down(gs::BTN_UP)) ay -= 1;
        if (pad.down(gs::BTN_DOWN)) ay += 1;
        if (std::fabs(pad.axisX) > 0.2f) ax = pad.axisX;
        if (std::fabs(pad.axisY) > 0.2f) ay = -pad.axisY;
        pressInk = pad.down(gs::BTN_A) || pad.down(gs::BTN_B);
    }
    ink_ = pressInk;

    float speed = bot_ ? 3.2f : 2.4f;
    float mag = std::sqrt(ax * ax + ay * ay);
    if (!bot_ && mag > 1.f) {
        ax /= mag;
        ay /= mag;
        mag = 1.f;
    }
    if (mag > 0.01f) {
        float step = bot_ ? std::min(speed, mag) : speed * std::min(mag, 1.f);
        float ux = ax / mag;
        float uy = ay / mag;
        x_ = std::clamp(x_ + ux * step, 40.f, 300.f);
        y_ = std::clamp(y_ + uy * step, 56.f, 200.f);
    }

    if (ink_ && seals_ < kNodes) {
        float dx = kPath[seals_].x - x_;
        float dy = kPath[seals_].y - y_;
        if (dx * dx + dy * dy < 12.f * 12.f) {
            x_ = kPath[seals_].x;
            y_ = kPath[seals_].y;
            seals_++;
            sys.apu.noiseBurst(0.05f, 900.f, 0.04f);
            if (seals_ >= kNodes) {
                won_ = true;
                mode_ = Mode::Done;
                hold_ = 90;
                ink_ = true;
            }
        }
    }

    bool moving = mag > 0.01f && ink_ && mode_ == Mode::Trace;
    if (moving) {
        scratch_ += 0.35f;
        sys.apu.tone(0, 110.f + 28.f * std::sin(scratch_), 0.035f);
    } else if (mode_ == Mode::Trace) {
        sys.apu.tone(0, 0, 0);
    }

    drawScene();
}

}  // namespace quillmark

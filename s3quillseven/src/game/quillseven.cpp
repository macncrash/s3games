#include "game/quillseven.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>

#include "console/gfx.h"

namespace quillseven {

namespace {

constexpr float kX0 = 36.f;
constexpr float kX1 = 284.f;
constexpr float kStep = 5.2f;
constexpr int kFaults = 7;

void pal(gs::VDP& vdp, int p, std::initializer_list<uint16_t> cs) {
    int i = 0;
    for (uint16_t c : cs) vdp.setColor(p * 16 + i++, c);
    while (i < 16) vdp.setColor(p * 16 + i++, 0);
}

gs::Bitmap quillBitmap() {
    gs::Bitmap b(20, 48);
    b.ellipse(7, 12, 6, 11, 3);
    b.ellipse(6, 11, 2, 6, 2);
    for (int i = 0; i < 5; i++) {
        float y = 6.f + float(i) * 3.f;
        b.line(7, y, 2, y + 2.f, 3, 1.f);
    }
    b.line(9, 6, 12, 42, 1, 2.f);
    b.line(12, 34, 15, 46, 4, 1.4f);
    b.line(11, 38, 14, 46, 5, 1.f);
    b.set(14, 45, 5);
    return b;
}

gs::Bitmap wellBitmap() {
    gs::Bitmap b(22, 14);
    b.ellipse(11, 8, 9, 5, 1);
    b.ellipse(11, 7, 5, 3, 2);
    b.ellipse(11, 7, 2, 1, 3);
    return b;
}

gs::Bitmap blotBitmap() {
    gs::Bitmap b(4, 4);
    b.ellipse(1.6f, 1.6f, 1.6f, 1.5f, 1);
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

float Game::guideY(float x) const {
    float t = (x - kX0) / (kX1 - kX0);
    t = std::max(0.f, std::min(1.f, t));
    float bend = (wave_ % 2 == 0) ? 26.f : -22.f;
    float mid = 108.f + float((wave_ / 2) % 3) * 6.f;
    return mid + std::sin(t * 3.14159f) * bend + std::sin(t * 6.2f + float(wave_)) * 6.f;
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    pal(vdp, 0, {0, gs::rgb4(15, 14, 11)});
    pal(vdp, 1, {0, gs::rgb4(2, 1, 3), gs::rgb4(6, 4, 8)});
    pal(vdp, 2, {0, gs::rgb4(5, 3, 1), gs::rgb4(13, 11, 7), gs::rgb4(15, 14, 12), gs::rgb4(12, 9, 3), gs::rgb4(3, 1, 1)});
    pal(vdp, 3, {0, gs::rgb4(9, 8, 6), gs::rgb4(7, 6, 4)});
    pal(vdp, 4, {0, gs::rgb4(8, 2, 2), gs::rgb4(14, 4, 3)});
    pal(vdp, 5, {0, gs::rgb4(2, 4, 8), gs::rgb4(4, 8, 14)});
    pal(vdp, 6, {0, gs::rgb4(15, 12, 3)});
    pal(vdp, 7, {0, gs::rgb4(4, 6, 3)});

    quill_ = gs::uploadImage(vdp, quillBitmap());
    well_ = gs::uploadImage(vdp, wellBitmap());
    blot_ = gs::uploadImage(vdp, blotBitmap());
    title_ = words(vdp, "S3 QUILL SEVEN", 2);
    hint_ = words(vdp, "FIRST TO SEVEN", 1);
    start_ = words(vdp, "PRESS START", 1);
    youWord_ = words(vdp, "YOU", 1);
    themWord_ = words(vdp, "THEM", 1);
    banner_ = words(vdp, "FIRST TO SEVEN", 2);
    lost_ = words(vdp, "THEY WROTE IT", 1);
    for (int d = 0; d < 10; d++) {
        char s[2] = {char('0' + d), 0};
        digit_[d] = words(vdp, s, 2);
    }

    gs::Bitmap desk(320, 224);
    desk.rect(0, 0, 320, 224, 2);
    desk.rect(12, 28, 296, 150, 1);
    desk.rect(18, 34, 284, 138, 4);
    uint32_t rng = 0x51A7u;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 16) & 255u;
    };
    for (int i = 0; i < 90; i++) desk.set(22 + int(rnd() % 272), 38 + int(rnd() % 126), 3);
    desk.rect(28, 158, 264, 2, 3);
    desk.ellipse(292, 46, 10, 7, 5);

    gs::TileAlloc tiles(vdp, 1);
    vdp.A.clear();
    vdp.B.clear();
    vdp.B.enabled = false;
    vdp.HUD.clear();
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, desk, 2);
    vdp.A.scroll(0, 0);
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.lineBackdrop[y] = gs::rgb4(4, 3, 2);
    vdp.setFogColor(gs::rgb4(4, 3, 2));
    for (int y = 0; y < gs::SCREEN_H; y++) vdp.road[y].on = false;
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildArt();
    x_ = 160.f;
    y_ = 120.f;
    sys.apu.setMaster(0.75f);
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
    spr(well_, 28, 196, yours_ && mode_ != Mode::Title ? 4 : 1);
    spr(well_, 292, 196, !yours_ && mode_ != Mode::Title ? 5 : 1);

    if (mode_ == Mode::Write || mode_ == Mode::Gap) {
        for (float x = kX0; x <= kX1; x += 10.f) spr(blot_, x, guideY(x), 3);
    }
    for (const Dot& d : ink_) spr(blot_, d.x, d.y, d.pal);

    float lift = (mode_ == Mode::Write) ? 0.f : 8.f;
    spr(quill_, x_, y_ - lift, 2, true);

    spr(youWord_, 36, 14, 4);
    spr(digit_[std::min(you_, 9)], 68, 14, 4);
    spr(themWord_, 250, 14, 5);
    spr(digit_[std::min(them_, 9)], 300, 14, 5);

    if (mode_ == Mode::Title) {
        spr(title_, 160, 78, 6);
        spr(hint_, 160, 104, 0);
        spr(start_, 160, 132, 0);
    } else if (mode_ == Mode::Done) {
        spr(won_ ? banner_ : lost_, 160, 78, won_ ? 6 : 0);
    }
}

void Game::beginStroke() {
    mode_ = Mode::Write;
    faults_ = 0;
    wave_ = strokes_;
    x_ = kX0;
    y_ = guideY(kX0);
    if (ink_.size() > 140) ink_.erase(ink_.begin(), ink_.begin() + 60);
}

void Game::finishStroke(bool hit) {
    if (hit) {
        if (yours_) you_++;
        else them_++;
        sys_->apu.tone(0, yours_ ? 440.f : 330.f, 0.08f);
    } else {
        sys_->apu.tone(0, 110.f, 0.06f);
    }
    strokes_++;
    yours_ = !yours_;
    if (you_ >= 7 && you_ > them_) {
        won_ = true;
        mode_ = Mode::Done;
        hold_ = 70;
        sys_->apu.tone(1, 523.f, 0.12f);
        sys_->apu.tone(2, 659.f, 0.1f);
        return;
    }
    if (them_ >= 7 && them_ > you_) {
        won_ = false;
        mode_ = Mode::Done;
        hold_ = 0;
        return;
    }
    mode_ = Mode::Gap;
    hold_ = 16;
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_ && sys.frame > 20) start = true;
        if (start) {
            you_ = them_ = strokes_ = 0;
            yours_ = true;
            won_ = over_ = false;
            ink_.clear();
            beginStroke();
            sys.apu.tone(0, 392.f, 0.07f);
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Done) {
        if (won_) {
            if (hold_ > 0) hold_--;
            if (hold_ == 0) {
                over_ = true;
                sys.quit();
            }
        } else if (pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A)) {
            mode_ = Mode::Title;
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Gap) {
        if (hold_ > 0) hold_--;
        if (hold_ == 0) beginStroke();
        float tx = yours_ ? 40.f : 280.f;
        x_ += (tx - x_) * 0.3f;
        y_ += (190.f - y_) * 0.3f;
        drawScene();
        return;
    }

    bool steerBot = bot_ || !yours_;
    float gy = guideY(x_);
    if (steerBot) {
        y_ += (gy - y_) * 0.85f;
    } else {
        if (pad.down(gs::BTN_UP)) y_ -= 2.4f;
        if (pad.down(gs::BTN_DOWN)) y_ += 2.4f;
        if (std::fabs(pad.axisY) > 0.2f) y_ -= pad.axisY * 2.6f;
    }
    if (std::fabs(y_ - gy) > 16.f) faults_++;
    int pal = yours_ ? 4 : 5;
    // Their hand misses two strokes in every six so a clean quill is first.
    bool scriptMiss = !yours_ && ((strokes_ % 6) == 3 || (strokes_ % 6) == 5);
    if (!scriptMiss) {
        ink_.push_back({x_, y_, pal});
    } else if ((strokes_ & 1) == 0) {
        faults_ += 3;
    }
    x_ += kStep;
    if ((int(x_) & 3) == 0) sys.apu.tone(0, yours_ ? 210.f : 160.f, 0.03f);
    if (x_ >= kX1) {
        bool hit = faults_ < kFaults && !scriptMiss;
        finishStroke(hit);
    }
    drawScene();
}

}  // namespace quillseven

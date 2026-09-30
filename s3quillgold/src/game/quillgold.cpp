#include "game/quillgold.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <string>

#include "console/gfx.h"

namespace quillgold {

namespace {

constexpr float kWellX[3] = {64.f, 160.f, 256.f};
constexpr float kWellY[3] = {178.f, 182.f, 178.f};
constexpr bool kWellGold[3] = {false, true, false};
// Cream, cream, then the gold finisher. Bare stays under the line.
constexpr int kBotWell[kMarks] = {0, 2, 1};

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
        float y = 7.f + float(i) * 3.1f;
        b.line(8, y, 2, y + 2.4f, 3, 1.f);
    }
    b.line(10, 8, 13, 46, 1, 2.2f);
    b.line(13, 38, 16, 50, 4, 1.5f);
    b.line(12, 42, 15, 50, 5, 1.f);
    b.set(15, 49, 5);
    return b;
}

gs::Bitmap wellBitmap() {
    gs::Bitmap b(26, 18);
    b.ellipse(13, 10, 11, 7, 1);
    b.ellipse(13, 9, 7, 4, 2);
    b.ellipse(13, 9, 3, 2, 3);
    return b;
}

gs::Bitmap blotBitmap() {
    gs::Bitmap b(5, 5);
    b.ellipse(2.2f, 2.2f, 2.1f, 2.0f, 1);
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

bool Game::paid() const {
    const int bare = gold_ + cream_;
    return finisherGold_ && gold_ >= 1 && score_ >= kLine && bare < kLine && score_ == gold_ * 2 + cream_;
}

void Game::buildMarks() {
    auto curve = [](std::vector<MarkPt>& out, float x0, float y0, float x1, float y1, float bend) {
        const int n = 16;
        for (int i = 0; i <= n; i++) {
            float t = float(i) / float(n);
            float x = x0 + (x1 - x0) * t;
            float y = y0 + (y1 - y0) * t + std::sin(t * 3.14159f) * bend;
            out.push_back({x, y});
        }
    };
    curve(marks_[0], 78, 78, 250, 70, -16);
    curve(marks_[1], 86, 108, 242, 116, 18);
    curve(marks_[2], 70, 142, 246, 132, -22);
}

void Game::buildArt() {
    gs::VDP& vdp = sys_->vdp;
    pal(vdp, 0, {0, gs::rgb4(15, 14, 12)});
    pal(vdp, 1, {0, gs::rgb4(1, 1, 2), gs::rgb4(4, 3, 6)});
    pal(vdp, 2, {0, gs::rgb4(4, 2, 1), gs::rgb4(12, 10, 6), gs::rgb4(15, 14, 11), gs::rgb4(13, 10, 4), gs::rgb4(2, 1, 1)});
    pal(vdp, 3, {0, gs::rgb4(10, 9, 7), gs::rgb4(8, 7, 5)});
    pal(vdp, 4, {0, gs::rgb4(14, 12, 8), gs::rgb4(12, 10, 7), gs::rgb4(9, 7, 4), gs::rgb4(15, 14, 11), gs::rgb4(11, 3, 2),
                 gs::rgb4(15, 12, 4)});
    pal(vdp, 5, {0, gs::rgb4(5, 3, 1), gs::rgb4(8, 6, 3), gs::rgb4(2, 1, 1)});
    pal(vdp, 6, {0, gs::rgb4(15, 12, 3), gs::rgb4(12, 8, 1)});
    pal(vdp, 7, {0, gs::rgb4(15, 14, 11), gs::rgb4(13, 12, 9)});

    quill_ = gs::uploadImage(vdp, quillBitmap());
    inkwell_ = gs::uploadImage(vdp, wellBitmap());
    blot_ = gs::uploadImage(vdp, blotBitmap());
    title_ = words(vdp, "S3 QUILL GOLD", 2);
    hint_ = words(vdp, "ONLY THE GOLD COUNTS DOUBLE", 1);
    start_ = words(vdp, "PRESS START", 1);
    pick_ = words(vdp, "A DIPS THE QUILL", 1);
    goldWord_ = words(vdp, "GOLD x2", 1);
    creamWord_ = words(vdp, "CREAM x1", 1);
    done_ = words(vdp, "GOLD DOUBLE", 2);
    ends_ = words(vdp, "ONLY THE GOLD COUNTS DOUBLE", 1);
    missed_ = words(vdp, "CREAM IS NOT THE DOUBLE", 1);
    lineImg_ = words(vdp, "LINE 4", 1);

    gs::Bitmap desk(320, 224);
    desk.rect(0, 0, 320, 224, 2);
    desk.rect(18, 18, 284, 150, 1);
    desk.rect(24, 24, 272, 138, 4);
    uint32_t rng = 0xC0FFEEu;
    auto rnd = [&]() {
        rng = rng * 1664525u + 1013904223u;
        return (rng >> 16) & 255u;
    };
    for (int i = 0; i < 110; i++) {
        int x = 28 + int(rnd() % 260);
        int y = 28 + int(rnd() % 126);
        desk.set(x, y, 3);
    }
    desk.rect(36, 148, 248, 2, 3);
    desk.ellipse(286, 36, 12, 9, 5);
    desk.ellipse(284, 34, 4, 3, 6);

    gs::TileAlloc tiles(vdp, 1);
    vdp.A.clear();
    vdp.B.clear();
    vdp.B.enabled = false;
    vdp.HUD.clear();
    gs::bitmapToPlane(tiles, vdp.A, 0, 0, desk, 4);
    vdp.A.scroll(0, 0);

    for (int y = 0; y < gs::SCREEN_H; y++) vdp.lineBackdrop[y] = gs::rgb4(3, 2, 1);
    vdp.setFogColor(gs::rgb4(3, 2, 1));
}

void Game::init(gs::System& sys) {
    sys_ = &sys;
    buildMarks();
    buildArt();
    x_ = kWellX[1];
    y_ = kWellY[1] - 20.f;
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

    for (int i = 0; i < 3; i++) {
        int pal = kWellGold[i] ? 6 : 7;
        if (mode_ == Mode::Pick && i == well_) pal = kWellGold[i] ? 6 : 5;
        spr(inkwell_, kWellX[i], kWellY[i], pal);
    }
    spr(creamWord_, kWellX[0], 204, 0);
    spr(goldWord_, kWellX[1], 206, 6);
    spr(creamWord_, kWellX[2], 204, 0);

    for (int m = 0; m < kMarks; m++) {
        bool open = m >= strokes_ && mode_ != Mode::Title;
        if (mode_ == Mode::Title) open = true;
        if (!open) continue;
        for (size_t i = 0; i < marks_[m].size(); i += 2) spr(blot_, marks_[m][i].x, marks_[m][i].y, 3);
    }
    for (const Dot& d : laid_) spr(blot_, d.x, d.y, d.pal);

    float lift = (mode_ == Mode::Write) ? 0.f : 6.f;
    spr(quill_, x_, y_ - lift, 2, true);

    if (mode_ == Mode::Title) {
        spr(title_, 160, 36, 6);
        spr(hint_, 160, 56, 0);
        spr(lineImg_, 160, 72, 0);
        spr(start_, 160, 154, 0);
    } else if (mode_ == Mode::Done) {
        spr(won_ ? done_ : missed_, 160, 40, won_ ? 6 : 0);
        spr(ends_, 160, 60, 0);
    } else if (mode_ == Mode::Pick) {
        spr(pick_, 160, 40, 0);
        spr(lineImg_, 160, 56, 0);
    }
}

void Game::finish(bool win) {
    won_ = win;
    say_ = win ? "GOLD DOUBLE" : "CREAM IS NOT THE DOUBLE";
    mode_ = Mode::Done;
    hold_ = 80;
    if (win) {
        sys_->apu.tone(0, 523.f, 0.14f);
        sys_->apu.tone(1, 659.f, 0.12f);
        sys_->apu.tone(2, 784.f, 0.1f);
    } else {
        sys_->apu.tone(0, 140.f, 0.12f);
    }
}

void Game::tally() {
    if (goldInk_) {
        gold_++;
        score_ += 2;
        finisherGold_ = true;
        say_ = "GOLD";
    } else {
        cream_++;
        score_ += 1;
        finisherGold_ = false;
        say_ = "CREAM";
    }
    strokes_++;
    sys_->apu.noiseBurst(0.08f, 700.f, 0.05f);
    if (paid()) finish(true);
    else if (strokes_ >= kMarks) finish(false);
    else {
        mode_ = Mode::Pick;
        wait_ = 0;
    }
}

void Game::commit() {
    if (strokes_ >= kMarks) return;
    mode_ = Mode::Write;
    cursor_ = 0;
    goldInk_ = kWellGold[well_];
    x_ = kWellX[well_];
    y_ = kWellY[well_];
    sys_->apu.noiseBurst(0.1f, 1400.f, 0.06f);
}

void Game::frame(gs::System& sys) {
    sys_ = &sys;
    gs::Pad& pad = sys.pad;

    if (mode_ == Mode::Title) {
        bool start = pad.pressed(gs::BTN_START) || pad.pressed(gs::BTN_A);
        if (bot_ && sys.frame > 24) start = true;
        if (start) {
            mode_ = Mode::Pick;
            well_ = 1;
            wait_ = 0;
            strokes_ = gold_ = cream_ = score_ = 0;
            finisherGold_ = false;
            laid_.clear();
            say_ = "SHORT QUILL";
            sys.apu.tone(0, 330.f, 0.08f);
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Done) {
        if (hold_ > 0) {
            hold_--;
            if (hold_ == 30) {
                sys.apu.tone(0, 0, 0);
                sys.apu.tone(1, 0, 0);
                sys.apu.tone(2, 0, 0);
            }
            if (hold_ == 0) over_ = true;
        }
        drawScene();
        return;
    }

    if (mode_ == Mode::Pick) {
        wait_++;
        if (bot_) {
            if (wait_ == 8) well_ = kBotWell[strokes_];
            if (wait_ > 16) commit();
        } else {
            if (pad.pressed(gs::BTN_LEFT)) well_ = (well_ + 2) % 3;
            if (pad.pressed(gs::BTN_RIGHT)) well_ = (well_ + 1) % 3;
            if (pad.pressed(gs::BTN_A) || pad.pressed(gs::BTN_B)) commit();
        }
        float tx = kWellX[well_];
        float ty = kWellY[well_] - 8.f;
        x_ += (tx - x_) * 0.35f;
        y_ += (ty - y_) * 0.35f;
        drawScene();
        return;
    }

    const std::vector<MarkPt>& path = marks_[strokes_];
    if (cursor_ >= int(path.size())) {
        tally();
        drawScene();
        return;
    }
    float tx = path[size_t(cursor_)].x;
    float ty = path[size_t(cursor_)].y;
    float dx = tx - x_;
    float dy = ty - y_;
    float dist = std::sqrt(dx * dx + dy * dy);
    float step = 7.5f;
    if (dist <= step) {
        x_ = tx;
        y_ = ty;
        int pal = goldInk_ ? 6 : 1;
        if (laid_.empty() || std::fabs(laid_.back().x - x_) + std::fabs(laid_.back().y - y_) > 3.f)
            laid_.push_back({x_, y_, pal});
        cursor_++;
        sys.apu.tone(0, goldInk_ ? 220.f : 160.f, 0.04f);
    } else {
        x_ += dx / dist * step;
        y_ += dy / dist * step;
    }
    drawScene();
}

}  // namespace quillgold

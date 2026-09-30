// S3 QUILL GOLD — a short quill. The line is 4.
// A gold stroke counts two. A cream stroke counts one.
// Cream cannot finish the line. Only the gold counts double.
#pragma once

#include <vector>

#include "console/system.h"

namespace quillgold {

constexpr int kLine = 4;
constexpr int kMarks = 3;

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUILL GOLD"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finisherGold() const { return finisherGold_; }
    int gold() const { return gold_; }
    int cream() const { return cream_; }
    int score() const { return score_; }
    int strokes() const { return strokes_; }
    int line() const { return kLine; }
    int bare() const { return gold_ + cream_; }
    const char* say() const { return say_; }

private:
    enum class Mode { Title, Pick, Write, Done };

    struct Dot {
        float x, y;
        int pal;
    };

    void buildArt();
    void buildMarks();
    void spr(const gs::Image& im, float x, float y, int pal, bool nib = false);
    void drawScene();
    bool paid() const;
    void commit();
    void tally();
    void finish(bool win);

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finisherGold_ = false;
    bool goldInk_ = false;
    int gold_ = 0;
    int cream_ = 0;
    int score_ = 0;
    int strokes_ = 0;
    int well_ = 1;
    int cursor_ = 0;
    int hold_ = 0;
    int wait_ = 0;
    float x_ = 160;
    float y_ = 120;
    const char* say_ = "SHORT QUILL";

    struct MarkPt {
        float x, y;
    };
    std::vector<MarkPt> marks_[kMarks];
    std::vector<Dot> laid_;

    gs::Image quill_{};
    gs::Image inkwell_{};
    gs::Image blot_{};
    gs::Image title_{};
    gs::Image hint_{};
    gs::Image start_{};
    gs::Image pick_{};
    gs::Image goldWord_{};
    gs::Image creamWord_{};
    gs::Image done_{};
    gs::Image ends_{};
    gs::Image missed_{};
    gs::Image lineImg_{};
};

}  // namespace quillgold

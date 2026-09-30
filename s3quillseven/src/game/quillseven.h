// S3 QUILL SEVEN — write until one side is first to seven, then leave.
#pragma once
#include <vector>

#include "console/system.h"

namespace quillseven {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUILL SEVEN"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int you() const { return you_; }
    int them() const { return them_; }
    int strokes() const { return strokes_; }

private:
    enum class Mode { Title, Write, Gap, Done };

    struct Dot {
        float x, y;
        int pal;
    };

    void buildArt();
    void spr(const gs::Image& im, float x, float y, int pal, bool nib = false);
    void drawScene();
    void beginStroke();
    void finishStroke(bool hit);
    float guideY(float x) const;

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool yours_ = true;
    int you_ = 0;
    int them_ = 0;
    int strokes_ = 0;
    int faults_ = 0;
    int hold_ = 0;
    int wave_ = 0;
    float x_ = 40.f;
    float y_ = 110.f;
    std::vector<Dot> ink_;

    gs::Image quill_{};
    gs::Image well_{};
    gs::Image blot_{};
    gs::Image title_{};
    gs::Image hint_{};
    gs::Image start_{};
    gs::Image youWord_{};
    gs::Image themWord_{};
    gs::Image banner_{};
    gs::Image lost_{};
    gs::Image digit_[10]{};
};

}  // namespace quillseven

// S3 QUILLMARK — trace the stroke. A finished mark ends it.
#pragma once
#include "console/system.h"

namespace quillmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUILLMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int seals() const { return seals_; }
    int marks() const { return marks_; }

private:
    enum class Mode { Title, Trace, Done };

    void buildArt();
    void drawScene();
    void spr(const gs::Image& im, float x, float y, int pal, bool nib = false);

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool ink_ = false;
    int seals_ = 1;
    int marks_ = 0;
    int hold_ = 0;
    float x_ = 0, y_ = 0;
    float scratch_ = 0;

    gs::Image quill_{};
    gs::Image well_{};
    gs::Image blot_{};
    gs::Image title_{};
    gs::Image hint_{};
    gs::Image start_{};
    gs::Image holdImg_{};
    gs::Image done_{};
    gs::Image ends_{};
    gs::Image digit_[10]{};
    gs::Image slash_{};
};

}  // namespace quillmark

// S3 QUILLBELL — ink the stroke. The bell rings before the third try dies.
#pragma once
#include "console/system.h"

namespace quillbell {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 QUILLBELL"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int dead() const { return dead_; }
    int seals() const { return seals_; }
    int marks() const { return marks_; }

private:
    enum class Mode { Title, Ink, Gap, Ring, Lose };

    void buildArt();
    void drawScene();
    void spr(const gs::Image& im, float x, float y, int pal, bool nib = false);
    void beginTry();
    void dieTry(const char* why);
    void ring();

    gs::System* sys_ = nullptr;
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool ink_ = false;
    bool wasInk_ = false;
    int seals_ = 1;
    int marks_ = 0;
    int dead_ = 0;
    int hold_ = 0;
    int chime_ = 0;
    float x_ = 0, y_ = 0;
    float swing_ = 0;
    float scratch_ = 0;
    const char* why_ = "";

    gs::Image quill_{};
    gs::Image well_{};
    gs::Image blot_{};
    gs::Image bell_{};
    gs::Image clapper_{};
    gs::Image title_{};
    gs::Image hint_{};
    gs::Image start_{};
    gs::Image holdImg_{};
    gs::Image rung_{};
    gs::Image died_{};
    gs::Image line_{};
    gs::Image digit_[10]{};
};

}  // namespace quillbell

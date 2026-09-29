// S3 CHOIRMARK — a short choir.
// Three voices have to meet on one gold bar. That finished mark ends it.
#pragma once
#include "console/system.h"
#include "game/art.h"

namespace choirmark {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CHOIRMARK"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool finished() const { return finished_; }
    int spread() const { return spread_; }
    int marker() const;

private:
    enum class Mode { Title, Phrase, Hold, Stop, Victory };

    void enterTitle();
    void begin();
    void cue(int i);
    bool stillOpen() const;
    bool arrived() const;
    int gap() const;
    float along(int i) const;
    bool window(int i) const;
    void chordOn();
    void hush();
    void phraseStep();
    void draw();
    void sky();
    void staff();
    void choir();
    void ink(int col, int row, const char* s, int pal);
    void inkC(int row, const char* s, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, int pal, bool feet, int fog = 0);
    void bar(int x, int y, int w, int h, int pal);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool finished_ = false;
    bool cued_[3] = {};
    int cueAt_[3] = {};
    int arr_[3] = {};
    int tick_ = 0;
    int hold_ = 0;
    int anim_ = 0;
    int spread_ = 0;
    int breath_ = 0;
};

}  // namespace choirmark

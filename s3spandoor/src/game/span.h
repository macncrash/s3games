// S3 SPANDOOR — at the span, hold the door for three minutes.
#pragma once
#include <cstdint>

#include "console/system.h"
#include "game/art.h"

namespace spandoor {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 SPANDOOR"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    int score() const { return score_; }
    // 0 title, 1 the span, 2 held, 3 the watch is over
    int marker() const;

private:
    enum class Mode { Title, Play, Pause, Won, Lost };
    static constexpr int SHEAR = 1;
    static constexpr int WAGON = 2;
    static constexpr int FLIP = 3;
    static constexpr int HOLD = 180 * 60;

    struct Mark {
        int frame;
        int kind;
    };
    struct Intent {
        float lean = 0;
        bool pin = false;
        bool shoulder = false;
    };

    void begin();
    void schedule();
    void update();
    void finish(bool kept);
    Intent intent();
    void draw();
    void backdrop();
    void hud(int col, int row, const char* s, int pal);
    void text(const char* s, float x, float y, float scale, int pal);
    void spr(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void stamp(const gs::Mipped& m, float cx, float cy, float w, float h, int pal, bool flip = false);
    void blip(float freq);

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    bool bot_ = false;
    bool won_ = false;
    bool over_ = false;
    bool pinned_ = false;
    bool wagonHeld_ = false;
    int age_ = 0;
    int score_ = 0;
    int pins_ = 0;
    int wagons_ = 0;
    int wind_ = 1;
    int gate_ = 0;
    int fanStep_ = -1;
    int markCount_ = 0;
    int markNext_ = 0;
    int shear_ = 0;
    int wagon_ = 0;
    int wagonPress_ = 0;
    float gap_ = 0;
    float lean_ = 1;
    float stam_ = 1;
    float shake_ = 0;
    float beep_ = 0;
    float fanT_ = 0;
    Mark marks_[48] = {};
};

}  // namespace spandoor

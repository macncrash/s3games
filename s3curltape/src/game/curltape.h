// S3 CURLTAPE — a short curl. The drawer has to match the tape.
// BUTTON, GUARD and BITE drop those slips in the till.
// FOUR, SHORT and RING pay the same and stay out.
#pragma once

#include "console/system.h"
#include "game/art.h"

namespace curltape {

class Game : public gs::Cart {
public:
    const char* title() const override { return "S3 CURLTAPE"; }
    void init(gs::System& sys) override;
    void frame(gs::System& sys) override;

    void setBot(bool on) { bot_ = on; }
    bool over() const { return over_; }
    bool won() const { return won_; }
    bool left() const { return left_; }
    bool matched() const { return held_[0] && held_[1] && held_[2]; }
    bool held(int i) const { return i >= 0 && i < 3 && held_[i]; }
    bool rules() const { return rules_; }
    bool sliding() const { return mode_ == Mode::Slide && slideFrames_ > 6 && slideFrames_ < 90; }
    int stones() const { return stones_; }
    int traps() const { return traps_; }
    int drawerScore() const;
    const char* tapeLabel(int i) const { return tapeName(i); }
    int tapeScore(int i) const { return tapePay(i); }
    const char* reason() const { return reason_; }
    const char* modeName() const;
    int phase() const;

private:
    enum class Mode { Title, Aim, Slide, Pocket, Judge, Leave, Lose, Pause, Over };

    struct Rock {
        float x = 0, y = 0, vx = 0, vy = 0;
        int handle = 1;
        bool dead = false;
    };
    struct End {
        float x = 0, y = 0;
        float vx = 0, vy = 0;
        int handle = 1;
        int frames = 0;
        bool dead = false;
        Lie lie = Lie::Miss;
        int pathN = 0;
        float pathX[16] = {};
        float pathY[16] = {};
    };
    struct Shot {
        float vx = 0, vy = 0.16f;
        int handle = 1;
    };
    struct Puff {
        float x = 0, y = 0, life = 0;
    };

    static void shove(Rock& r, float sweep);
    static End fly(float vx, float vy, int handle, bool record);
    bool solve();
    bool audit();
    void showTitle();
    void newGame();
    void beginAim();
    void launch();
    void stepSlide();
    void settle();
    void beginLeave();
    void beginLose();
    void afterPocket();
    void afterJudge();
    int nextOpen() const;
    void predict();
    void botAim();
    void humanAim(const gs::Pad& pad);
    void blip(float freq);
    void tickAudio();
    void spr(const gs::Image& img, float cx, float cy, float w, float h, int pal, bool flip = false,
             bool shadow = false);
    void sprM(const gs::Mipped& m, float cx, float cy, float h, int pal, bool flip = false);
    void hud(int col, int row, const char* s, int pal);
    void hudC(int row, const char* s, int pal);
    void chrome();
    void draw();

    gs::System* sys_ = nullptr;
    Art art_{};
    Mode mode_ = Mode::Title;
    Mode heldMode_ = Mode::Aim;
    Rock rock_{};
    End preview_{};
    Shot shot_[3]{};
    Puff puff_[6]{};
    char reason_[48] = {};
    bool bot_ = false;
    bool over_ = false;
    bool won_ = false;
    bool left_ = false;
    bool rules_ = false;
    bool held_[3] = {};
    int stones_ = 0;
    int traps_ = 0;
    int slideFrames_ = 0;
    int shake_ = 0;
    int beep_ = 0;
    int think_ = 0;
    int holdX_ = 0;
    int holdY_ = 0;
    int slipI_ = -1;
    int puffN_ = 0;
    float aimVx_ = 0;
    float aimVy_ = 0.16f;
    int handle_ = 1;
    float sweep_ = 0;
    float clock_ = 0;
    float pocketT_ = 0;
    float judgeT_ = 0;
    float leaveT_ = 0;
    float slipSX_ = 0;
    float slipSY_ = 0;
    Lie lastLie_ = Lie::Miss;
};

}  // namespace curltape
